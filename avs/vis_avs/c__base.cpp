#include "c__base.h"

#include "avs_eelif.h"

CodeSection::CodeSection() : string(new char[1]), need_recompile(false), code(NULL) {
    this->string[0] = '\0';
}

CodeSection::~CodeSection() {
    delete[] this->string;
    NSEEL_code_free(this->code);
    this->code = NULL;
}

/* `length` must include the zero byte! */
void CodeSection::set(char* string, unsigned int length) {
    delete[] this->string;
    length = length == 0 ? 1 : length;
    this->string = new char[length];
    strncpy(this->string, string, length - 1);
    this->string[length - 1] = '\0';
    this->need_recompile = true;
}

bool CodeSection::recompile_if_needed(void* vm_context) {
    if (!this->need_recompile) {
        return false;
    }
    NSEEL_code_free(this->code);
    this->code = NSEEL_code_compile(vm_context, this->string);
    this->need_recompile = false;
    return true;
}

void CodeSection::exec(char visdata[2][2][576]) {
    if (!this->code) {
        return;
    }
    AVS_EEL_IF_Execute(code, visdata);
}

int CodeSection::load(char* src, u_int max_len) {
    u_int code_len = strnlen(src, max_len);
    this->set(src, code_len + 1);
    return code_len + 1;
}

int CodeSection::save(char* dest, u_int max_len) {
    u_int code_len = strnlen(this->string, max_len);
    bool code_too_long = code_len == max_len;
    if (code_too_long) {
        strncpy(dest, this->string, code_len);
        dest[code_len] = '\0';
    } else {
        strncpy(dest, this->string, code_len + 1);
    }
    return code_len + 1;
}

ComponentCodeBase::ComponentCodeBase() : vm_context(NULL) {}

ComponentCodeBase::~ComponentCodeBase() { AVS_EEL_IF_VM_free(this->vm_context); }

void ComponentCodeBase::recompile_if_needed() {
    if (this->vm_context == NULL) {
        this->vm_context = NSEEL_VM_alloc();
        if (this->vm_context == NULL) {
            return;
        }
        this->register_variables();
    }
    if (this->init.recompile_if_needed(this->vm_context)) {
        this->need_init = true;
    }
    this->frame.recompile_if_needed(this->vm_context);
    this->beat.recompile_if_needed(this->vm_context);
    this->point.recompile_if_needed(this->vm_context);
}
