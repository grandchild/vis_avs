pub trait FromCEnum<T>
where
    Self: Sized,
{
    fn from_value(value: T) -> Option<Self>;
    fn to_value(&self) -> T;
}
