use std::collections::VecDeque;

#[derive(Debug, Default)]
pub(crate) struct InsertOnlyTree<T>
where
    T: PartialEq,
{
    arena: Vec<Node<T>>,
}

#[derive(Debug)]
struct Node<T>
where
    T: PartialEq,
{
    id: usize,
    val: T,
    parent: Option<usize>,
    children: Vec<usize>,
}

impl<T> InsertOnlyTree<T>
where
    T: PartialEq,
{
    pub(crate) fn new() -> Self {
        Self { arena: Vec::new() }
    }

    pub(crate) fn clear(&mut self) {
        self.arena.clear();
    }

    fn get_id(&self, val: &T) -> Option<usize> {
        self.arena.iter().find(|node| node.val == *val).map(|n| n.id)
    }

    // pub(crate) fn get(&self, id: usize) -> Option<&T> {
    //     self.arena.get(id).map(|node| &node.val)
    // }

    fn valid_parent(&self, id: Option<usize>) -> bool {
        match id {
            Some(id) => self.arena.len() > id,
            None => true, // The root's parent is None, and always exists
        }
    }

    pub(crate) fn insert(&mut self, parent: Option<usize>, val: T) -> Option<usize> {
        if !self.valid_parent(parent) {
            return None;
        }
        let id = self.arena.len();
        self.arena.push(Node { id, val, parent: None, children: Vec::new() });
        if let Some(parent_id) = parent {
            self.arena[id].parent = parent;
            self.arena[parent_id].children.push(id);
        }
        Some(id)
    }

    pub(crate) fn root(&self) -> &T {
        &self.arena[0].val
    }

    pub(crate) fn parent(&self, val: &T) -> Option<&T> {
        self.get_id(val)
            .and_then(|id| self.arena[id].parent)
            .map(|parent_id| &self.arena[parent_id].val)
    }

    pub(crate) fn children(&self, val: &T) -> Vec<&T> {
        self.get_id(val)
            .map(|id| {
                self.arena[id].children.iter().map(|&id| &self.arena[id].val).collect()
            })
            .unwrap_or_default()
    }

    pub(crate) fn iter(&self) -> TreeIter<T> {
        TreeIter::new(self)
    }
}

// impl<T: std::cmp::PartialEq> Iterator for InsertOnlyTree<T> {
//     type Item = (usize, &T);

//     fn next(&mut self) -> Option<Self::Item> {
//         Some((0, &self.root()))
//     }
// }

pub(crate) struct TreeIter<'a, T>
where
    T: PartialEq,
{
    tree: &'a InsertOnlyTree<T>,
    queue: VecDeque<TreeIterItem>,
}

struct TreeIterItem {
    id: usize,
    depth: usize,
}

impl<'a, T> TreeIter<'a, T>
where
    T: PartialEq,
{
    pub fn new(tree: &'a InsertOnlyTree<T>) -> Self {
        Self { tree, queue: vec![TreeIterItem { id: 0, depth: 0 }].into() }
    }
}

impl<'a, T> Iterator for TreeIter<'a, T>
where
    T: PartialEq,
{
    type Item = (usize, &'a T);

    fn next(&mut self) -> Option<Self::Item> {
        let TreeIterItem { id, depth } = self.queue.pop_front()?;
        let node = &self.tree.arena[id];
        for id in node.children.iter().rev() {
            self.queue.push_front(TreeIterItem { id: *id, depth: depth + 1 })
        }
        Some((depth, &node.val))
    }
}
