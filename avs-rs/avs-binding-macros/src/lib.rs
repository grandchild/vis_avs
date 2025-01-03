use proc_macro::TokenStream;
// use proc_macro::TokenTree;
// use std::str::FromStr;

use quote::quote;
use syn::{parse, Data, DeriveInput, Expr, Ident};

#[proc_macro_derive(FromCEnum)]
pub fn from_c_enum_derive(input: TokenStream) -> TokenStream {
    match from_c_enum_derive_impl(input) {
        Ok(output) => output,
        Err(err) => {
            let err_str = err.to_string();
            quote! {compile_error!(#err_str)}.into()
        }
    }
}

fn from_c_enum_derive_impl(
    input: TokenStream,
) -> Result<TokenStream, Box<dyn std::error::Error>> {
    let ast: DeriveInput = parse(input)?;
    let enum_name = &ast.ident;
    let repr = ast.attrs.iter().find_map(|attr| {
        if attr.path().is_ident("repr") {
            let type_ident: Ident = attr.parse_args().ok()?;
            return Some(type_ident);
        }
        None
    });
    if repr.is_none() {
        return Err(format!("Must set #[repr(TYPE)] attribute on {enum_name}").into());
    }
    let mut variant_idents: Vec<&Ident> = Vec::new();
    let mut variant_discriminants: Vec<Option<&Expr>> = Vec::new();
    if let Data::Enum(ref data) = ast.data {
        for variant in &data.variants {
            variant_idents.push(&variant.ident);
            variant_discriminants
                .push(variant.discriminant.as_ref().map(|(_, expr)| expr));
        }
    }
    Ok(quote! {
        impl FromCEnum<#repr> for #enum_name {
            fn from_value(value: #repr) -> Option<Self> {
                match value {
                    #(#variant_discriminants => Some(#enum_name::#variant_idents),)*
                    _ => None,
                }
            }
            fn to_value(&self) -> #repr {
                match self {
                    #(#enum_name::#variant_idents => #variant_discriminants,)*
                }
            }
        }
    }
    .into())
}
