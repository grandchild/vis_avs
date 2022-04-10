#include "effect.h"

#include "../platform.h"  // min/max

#include <stdint.h>

Effect::~Effect() {
    for (auto child : this->children) {
        delete child;
    }
}

Effect* Effect::insert(Effect* to_insert,
                       Effect* relative_to,
                       Effect::Insert_Direction direction) {
    if (relative_to == this) {
        if (direction == INSERT_CHILD) {
            this->children.push_back(to_insert);
            return to_insert;
        } else {
            return NULL;
        }
    }
    for (auto child = this->children.begin(); child != this->children.end(); child++) {
        if (*child == relative_to) {
            switch (direction) {
                case INSERT_BEFORE: this->children.insert(child, to_insert); break;
                case INSERT_AFTER: this->children.insert(++child, to_insert); break;
                case INSERT_CHILD:
                    // Note: this one is not vector::insert(), but a recursive call
                    (*child)->insert(to_insert, *child, direction);
                    break;
            }
            return to_insert;
        }
        Effect* subtree_result = (*child)->insert(to_insert, relative_to, direction);
        if (subtree_result != NULL) {
            return subtree_result;
        }
    }
    return NULL;
}

Effect* Effect::lift(Effect* to_lift) {
    if (to_lift == this) {
        return this;
    }
    Effect* lifted = NULL;
    for (auto child = this->children.begin(); child != this->children.end();) {
        if (*child == to_lift) {
            lifted = *child;
            child = this->children.erase(child);
            break;
        } else {
            child++;
        }
        Effect* subtree_result = (*child)->lift(to_lift);
        if (subtree_result != NULL) {
            return subtree_result;
        }
    }
    return lifted;
}

Effect* Effect::move(Effect* to_move,
                     Effect* relative_to,
                     Effect::Insert_Direction direction) {
    Effect* lifted = this->lift(to_move);
    if (lifted == NULL) {
        return NULL;
    }
    return this->insert(lifted, relative_to, direction);
}

void Effect::remove(Effect* to_remove) {
    if (to_remove == this) {
        return;
    }
    delete this->lift(to_remove);
}

Effect* Effect::duplicate(Effect* to_duplicate) {
    if (to_duplicate == this) {
        return NULL;
    }
    for (auto child = this->children.begin(); child != this->children.end(); child++) {
        if (*child == to_duplicate) {
            Effect* duplicate_ = to_duplicate->duplicate_with_children();
            this->children.insert(++child, duplicate_);
            return duplicate_;
        }
        Effect* subtree_result = (*child)->duplicate(to_duplicate);
        if (subtree_result != NULL) {
            return subtree_result;
        }
    }
    return NULL;
}
Effect* Effect::duplicate_with_children() {
    Effect* duplicate_ = this->clone();
    for (auto child : this->children) {
        this->children.push_back(child->duplicate_with_children());
    }
    return duplicate_;
}

void Effect::set_enabled(bool enabled) {
    this->enabled = enabled;
    this->on_enable(enabled);
}

Effect* Effect::find_by_handle(AVS_Component_Handle handle) {
    if (this->handle == handle) {
        return this;
    }
    for (auto child : this->children) {
        Effect* subtree_result = child->find_by_handle(handle);
        if (subtree_result != NULL) {
            return subtree_result;
        }
    }
    return NULL;
}

const AVS_Component_Handle* Effect::get_child_handles_for_api() {
    AVS_Component_Handle* old_handles = this->child_handles_for_api;
    AVS_Component_Handle* new_handles = new AVS_Component_Handle[this->children.size()];
    for (size_t i = 0; i < this->children.size(); ++i) {
        new_handles[i] = this->children[i]->handle;
    }
    delete[] old_handles;
    this->child_handles_for_api = new_handles;
    return this->child_handles_for_api;
}

uint32_t Effect::string_load_legacy(const char* src,
                                    std::string& dest,
                                    uint32_t max_length) {
    if (max_length < sizeof(uint32_t)) {
        return 0;
    }
    uint32_t size = *((uint32_t*)src);
    uint32_t pos = sizeof(uint32_t);
    if (size > 0 && max_length >= size) {
        dest.assign(&src[pos], size);
        pos += size;
        // So far so good. But some older effects's save format has length-prefixed
        // strings which _include_ the null-terminator. If that's the case remove
        // anything including and after it.
        size_t first_null_byte = dest.find_first_of('\0');
        if (first_null_byte != std::string::npos) {
            dest.resize(first_null_byte);
        }
    } else {
        dest.assign("");
    }
    return pos;
}

uint32_t Effect::string_save_legacy(std::string& src,
                                    char* dest,
                                    uint32_t max_length,
                                    bool with_nt) {
    if (!src.empty()) {
        if (src.length() > max_length) {
            printf("string truncated for legacy save format\n");
        }
        size_t length = min(max_length, src.length() + (with_nt ? 1 : 0));
        *((uint32_t*)dest) = length;
        uint32_t pos = sizeof(uint32_t);
        memcpy(dest + pos, src.c_str(), length);
        pos += length;
        return pos;
    } else {
        *((uint32_t*)dest) = 0;
        return sizeof(uint32_t);
    }
}

uint32_t Effect::string_nt_load_legacy(const char* src,
                                       std::string& dest,
                                       uint32_t max_length) {
    if (max_length < 1) {
        return 0;
    }
    auto code_len = strnlen(src, max_length);
    dest.assign(src, code_len);
    return code_len + 1;
}

uint32_t Effect::string_nt_save_legacy(std::string& src,
                                       char* dest,
                                       uint32_t max_length) {
    if (max_length < 1) {
        return 0;
    }
    auto code_len = src.length();
    if (code_len > max_length) {
        code_len = max_length;
    }
    strncpy(dest, src.c_str(), code_len + 1);
    dest[code_len] = '\0';
    return code_len + 1;
}
