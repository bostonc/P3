#include "btree.h"
#include "bnode.h"
#include "bnode_inner.h"
#include "bnode_leaf.h"

#include <cassert>

using namespace std;

const int LEAF_ORDER = BTREE_LEAF_SIZE/2;
const int INNER_ORDER = (BTREE_FANOUT-1)/2;

Btree::Btree() : root(new Bnode_leaf), size(0) {
    // Fill in here if needed
}

Btree::~Btree() {
    // Don't forget to deallocate memory
}

bool Btree::insert(VALUETYPE value) 
{
    // TODO: Implement this
	assert(root);
	size++;

	//find node where new value should go
	Bnode* current = root;
	Bnode_inner* inner = dynamic_cast<Bnode_inner*>(current);
	while (inner)		//while current isn't a leaf...
	{
		//which child should have this new value?
		int child_idx = inner->find_value_gt(value);
		current = inner->getChild(child_idx);
		inner = dynamic_cast<Bnode_inner*>(current);

	}
	//found the leaf node where the new value belongs :)
	Bnode_leaf* leaf = dynamic_cast<Bnode_leaf*>(current);
	assert(leaf);

	//if node is not full, insert and return
	if (!leaf->is_full()) return;

	//DO-WHILE LOOP????????????????????????????????????????????????????????????????

	//from here, node must be full. SPLIT.
		//do split
	Bnode_leaf* new_leaf = leaf->split(value); 
		//reassign parent value
	int reassignment_idx = new_leaf->parent->find_value_gt(value);
	new_leaf->parent->replace_value(value, reassignment_idx); //has potential to break things

	//now we need to give this new_leaf a parent. Does the old one have room?
	if (!leaf->parent->is_full())
	{
		leaf->parent->insert(new_leaf, reassignment_idx + 1); //off by 1?
		//return;
	}
	else //old parent is full.
	{

	}


	// Try to FIX SOMEHOW WITH A HTING........................................


	//did we break the tree?
	if (isValid()) return;

	//...we broke the tree :(








    return true;
}

bool Btree::remove(VALUETYPE value) {
    // TODO: Implement this
    return true;
}

vector<Data*> Btree::search_range(VALUETYPE begin, VALUETYPE end) {
    std::vector<Data*> returnValues;
    // TODO: Implement this

    return returnValues;
}

//
// Given code
//
Data* Btree::search(VALUETYPE value) {
    assert(root);
    Bnode* current = root;

    // Have not reached a leaf node yet
    Bnode_inner* inner = dynamic_cast<Bnode_inner*>(current);
    // A dynamic cast <T> will return a nullptr if the given input is polymorphically a T
    //                    will return a upcasted pointer to a T* if given input is polymorphically a T
    while (inner) {
        int find_index = inner->find_value_gt(value);
        current = inner->getChild(find_index);
        inner = dynamic_cast<Bnode_inner*>(current);
    }

    // Found a leaf node
    Bnode_leaf* leaf = dynamic_cast<Bnode_leaf*>(current);
    assert(leaf);
    for (int i = 0; i < leaf->getNumValues(); ++i) {
        if (leaf->get(i) > value)    return nullptr; // passed the possible location
        if (leaf->get(i) == value)   return leaf->getData(i);
    }

    // reached past the possible values - not here
    return nullptr;
}

