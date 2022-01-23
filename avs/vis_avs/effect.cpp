#include "effect.h"

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
                case INSERT_BEFORE:
                    this->children.insert(child, to_insert);
                    break;
                case INSERT_AFTER:
                    this->children.insert(++child, to_insert);
                    break;
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
