#include "btree.h"
#include "bnode.h"
#include "bnode_inner.h"
#include "bnode_leaf.h"
#include <vector>
#include <cassert>
#include <iostream> // for debugging

using namespace std;

const int LEAF_ORDER = BTREE_LEAF_SIZE/2;
const int INNER_ORDER = (BTREE_FANOUT-1)/2;

Btree::Btree() : root(new Bnode_leaf), size(0) {
    // Fill in here if needed
}

Btree::~Btree() 
{
    //Don't forget to deallocate memory
	//Uses modified in-order traversal to find all nodes, then delete them all
	vector<VALUETYPE> vals;
	vector<Bnode*> ptrs;
	dtor_traverse(root, vals, ptrs);

	for (int i = 0; i < ptrs.size(); ++i)
	{
		delete ptrs[i];
		ptrs[i] = nullptr;
	}
}

void Btree::dtor_traverse(Bnode* current, vector<VALUETYPE>& values, vector<Bnode*>& ptrs)
{
	Bnode_inner* inner = dynamic_cast<Bnode_inner*>(current);
	if (inner) {
		ptrs.push_back(current);
		assert(inner->getNumChildren() != 0);
		assert(inner->getNumValues() == inner->getNumChildren() - 1);
		dtor_traverse(inner->getChild(0), values, ptrs);
		for (int i = 0; i < inner->getNumValues(); ++i) {
			values.push_back(inner->get(i));
			dtor_traverse(inner->getChild(i + 1), values, ptrs);
		}
	}
	else {
		ptrs.push_back(current);
		// not a inner? must be a leaf
		Bnode_leaf* leaf = dynamic_cast<Bnode_leaf*>(current);
		assert(leaf);
		for (int i = 0; i < leaf->getNumValues(); ++i) {
			values.push_back(leaf->get(i));
		}
	}
}

bool Btree::insert(VALUETYPE value) 
{
    // TODO: Implement this
	assert(root);

	//SEARCH FOR EXISTING VALUE
	if (search(value) != nullptr) return false;

	//INSERT
	size++;
	Data* new_data = new Data(value);

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
	//found the leaf node in which the new value belongs :)
	Bnode_leaf* leaf = dynamic_cast<Bnode_leaf*>(current);
	assert(leaf);	

	//if node is not full, insert and return
	if (!leaf->is_full())
	{
		leaf->insert(new_data);
		assert(isValid());
		assert(search(value));
		return true;
	}	

	//from here, node must be full. SPLIT.
		//do split
	Bnode_leaf* new_leaf = leaf->split(value); 	
	
		//Did we split the root?
	if (root == leaf)
	{
		//make new root, return
		Bnode_inner* new_root = new Bnode_inner();
			//values
		new_root->insert(new_leaf->get(0));
			//children
		new_root->insert(leaf, 0);
		new_root->insert(new_leaf, 1);		
			//parents
		leaf->parent = new_root;
		new_leaf->parent = new_root;

		root = new_root;
		assert(isValid());
		return true;
	}		

	int reassignment_idx = -1;
	//now we need to give this new_leaf a parent. Does the old one have room?
	if (!leaf->parent->is_full())
	{
		//add parent value
		reassignment_idx = new_leaf->parent->find_value_gt(value);
		//new_leaf->parent->replace_value(new_leaf->get(0), reassignment_idx);
		new_leaf->parent->insert(new_leaf->get(0));
		
		leaf->parent->insert(new_leaf, reassignment_idx + 1); //off by 1? +1 maybe
		assert(isValid());
		return true;
	}

	//else parent is full. SPLIT them, too.
	Bnode* child_waiting = new_leaf;
	VALUETYPE insert_value = new_leaf->get(0);
	VALUETYPE out = -1;
	Bnode_inner* new_parent = nullptr;
	bool rootSplit = false;
	
	while (true)
	{	
		//NEED TO CHECK FOR ROOT SPLIT BEFORE WE ACTUALLY SPLIT...
		if (root == child_waiting->parent) rootSplit = true;

		new_parent = child_waiting->parent->split(out, insert_value, child_waiting);		

		//if we split the root, make a new root and return.
		if (rootSplit)
		{
			//make new root, return
			Bnode_inner* new_root = new Bnode_inner();
				//values
			new_root->insert(out);
				//children
			new_root->insert(root, 0);
			new_root->insert(new_parent, 1);
				//parents
			root->parent = new_root;
			new_parent->parent = new_root;
				//move root up
			root = new_root;
			
			assert(isValid());
			return true;
		}

		//else, check next level up and see if they have room for new node ptr		
		if (!new_parent->parent->is_full())
		{
			//add and return
			//reassignment_idx = new_parent->parent->find_value_gt(out);
			//new_parent->parent->replace_value(out, reassignment_idx);
			new_parent->parent->insert(out); //value

			reassignment_idx = new_parent->parent->find_value_gt(out);
			new_parent->parent->insert(new_parent, reassignment_idx); //child
			assert(isValid());
			assert(search(value));
			return true;
		}

		//else, prep and continue upwards split
		child_waiting = new_parent;
		insert_value = out;
	}//end while

	//If ever this code is reached, I will eat my cat
	assert(isValid());
	assert(false);
	return false;
}


	
bool Btree::remove(VALUETYPE value)
{
	assert(root);
	assert(isValid());

	//Return false if value doesn't exist
	if (!search(value)) return false;

	size--;

	//find leaf which holds value
	Bnode* current = root;
	Bnode_inner* inner = dynamic_cast<Bnode_inner*>(current);
	while (inner)		//while current isn't a leaf...
	{
		//which child has the value?
		int child_idx = inner->find_value_gt(value);
		current = inner->getChild(child_idx);
		inner = dynamic_cast<Bnode_inner*>(current);
	}
	//found it :)
	Bnode_leaf* leaf = dynamic_cast<Bnode_leaf*>(current);
	assert(leaf);

	//remove value
	leaf->remove(value);

	//if leaf is still half full, return
	assert(isValid());
	if (leaf->at_least_half_full()) return true;

	//if this is the root, return
	assert(isValid());
	if (root == leaf) return true;

	//from here, leaf is less than half full and not the root :(

	//can we redistribute?
	VALUETYPE out = -1;
	//can we redistribute right? check right node for extra values
	if (leaf->next && leaf->next->getNumValues() > (BTREE_LEAF_SIZE / 2))
	{
		out = leaf->redistribute(leaf->next);
		//reassign value of common ancestor
		Bnode_inner* ancestor = leaf->common_ancestor(leaf->next);
		int idx = ancestor->find_value_gt(out) - 1; //OFF BY 1????????????????
		ancestor->replace_value(out, idx); //need to check if necessary???????
		assert(isValid());
		return true;
	}
	//else can we redistribute left? if right didn't work, check left
	if (leaf->prev && leaf->prev->getNumValues() > (BTREE_LEAF_SIZE / 2))
	{
		out = leaf->prev->redistribute(leaf);
		//reassign value of common ancestor
		Bnode_inner* ancestor = leaf->prev->common_ancestor(leaf);
		int idx = ancestor->find_value_gt(out); //OFF BY 1?????????? //makes remove 11 work
		ancestor->replace_value(out, idx);
		assert(isValid());
		return true;
	}

	//we can't redistribute :(		

	Bnode_inner* underfilled = nullptr;
	//lets MERGE leaf nodes.
	//can we merge with right?
	if (leaf->next)
	{
		Bnode_inner* rightParent = leaf->next->parent;
		int idx = rightParent->find_child(leaf->next);
		cout << "leaf->next values: ";
		for (int i = 0; i < leaf->next->getNumValues(); i++) {
			cout << leaf->next->get(i) << " ";
		}
		cout << "rightParent num children: " << rightParent->getNumChildren() << endl;
		cout << "leaf parent num children: " << leaf->parent->getNumChildren() << endl;
		cout << endl;
		out = leaf->merge(leaf->next);
		//fix parent of right node, and commmon ancestor if different
		cout << "before remove child line 266" << endl;
		cout << "idx = " << idx << endl;
		cout << "rightParent values: ";
		for (int i = 0; i < rightParent->getNumValues(); i++) {
			cout << rightParent->get(i) << " ";
		}
		cout << endl;
		
		rightParent->remove_child(idx); //MAKE SURE NOT TO DO THIS IN MERGE. MEMORY LEAK?????
		cout << "after remove child line 266" << endl;
		//if we merged siblings...
		if (leaf->parent == rightParent) rightParent->remove_value(idx - 1);
		//if we merged non-siblings...
		if (leaf->parent != rightParent)
		{
			out = rightParent->get(0); //added this to fix remove 8
			rightParent->remove_value(0);
			//fix value of common ancestor
			idx = leaf->parent->common_ancestor(rightParent)->find_value_gt(out) - 1;
			leaf->parent->common_ancestor(rightParent)->replace_value(out, idx);
		}
		//see if rightParent is underfilled now, return if we're good
		if (rightParent->at_least_half_full())
		{
			assert(isValid()); //BREAKS THINGS
			return true;
		}
		underfilled = rightParent;
	}
	//else, we merge with the left
	else if (leaf->prev)
	{
		Bnode_inner* rightParent = leaf->parent;
		int idx = rightParent->find_child(leaf);
		out = leaf->prev->merge(leaf);
		//fix parent of right node, and common ancestor if different
		cout << "before remove child line 295" << endl;
		rightParent->remove_child(idx); //MAKE SURE NOT TO DO THIS IN MERGE. MEMORY LEAK?????
		cout << "after remove child line 295" << endl;
		//if we merged siblings...
		if (leaf->prev->parent == rightParent) rightParent->remove_value(idx - 1);
		//if we merged non-siblings...
		if (leaf->prev->parent != rightParent)
		{
			rightParent->remove_value(0);
			//fix value of common ancestor
			idx = leaf->prev->parent->common_ancestor(rightParent)->find_value_gt(out) - 1;
			leaf->prev->parent->common_ancestor(rightParent)->replace_value(out, idx);
		}
		//see if rightParent is underfilled now, return if we're good
		if (rightParent->at_least_half_full())
		{
			assert(isValid());
			return true;
		}
		underfilled = rightParent;
	}

	//damn, something higher up must be underfilled...

	//LOOP
	//1) root check
	//2) redist (only siblings now)
	//3) merge (only siblings now)
	while (true)
	{
		if (underfilled == root)
		{
			if (underfilled->getNumChildren() >= 2 || underfilled->getNumChildren() == 0) return true;
			else //root has one child. needs to be removed and moved down
			{
				Bnode* target = root;
				root = underfilled->getChild(0);
				delete target;
				assert(isValid());
				return true;
			}
		}
		assert(underfilled->getNumChildren() > 0); //changed 1 to 0
		//underfilled isn't the root.
		int underfilled_idx = underfilled->parent->find_child(underfilled);
		Bnode_inner* rightSibling = nullptr;
		Bnode_inner* leftSibling = nullptr;

		//can we REDISTRIBUTE?
		//check right...
		//does sibling exist?
		if (underfilled->parent->getNumChildren() > underfilled_idx + 1)
		{
			rightSibling = dynamic_cast<Bnode_inner*>(underfilled->parent->getChild(underfilled_idx + 1));
			assert(underfilled->is_sibling_of(rightSibling));
		}
		//does sibling have values to spare?
		if (rightSibling && rightSibling->getNumValues() > (BTREE_FANOUT - 1) / 2)
		{	//REDISTRIBUTE with right
			out = underfilled->redistribute(rightSibling, underfilled_idx);
			
			//reassign parent value - THIS COULD BREAK ROTATION FUNCTIONALITY if out is wrong.
			underfilled->parent->replace_value(out, underfilled_idx);
			
			assert(isValid());
			return true;
		}
		//else, check left...
		//does sibling exist?
		if (underfilled_idx > 0)
		{
			leftSibling = dynamic_cast<Bnode_inner*>(underfilled->parent->getChild(underfilled_idx - 1));
			assert(underfilled->is_sibling_of(leftSibling));
		}
		//does sibling have values to spare?
		if (leftSibling && leftSibling->getNumValues() > (BTREE_FANOUT - 1) / 2)
		{	//REDISTRIBUTE left
			out = leftSibling->redistribute(underfilled, underfilled_idx - 1);
			//reassign parent value - THIS COULD BREAK ROTATION FUNCTIONALITY if out is wrong.
			underfilled->parent->replace_value(out, underfilled_idx - 1);
			assert(isValid());
			return true;
		}

		//can we MERGE?
		//check right
		if (rightSibling &&
			underfilled->getNumValues() + rightSibling->getNumValues() < BTREE_FANOUT - 1)
		{	//MERGE right
			out = underfilled->merge(rightSibling, underfilled_idx);
			//fix parent
			cout << "before remove child line 386" << endl;
			underfilled->parent->remove_child(underfilled_idx + 1);
			cout << "after remove child line 386" << endl;
			underfilled->parent->remove_value(underfilled_idx);
			//did we underfill the parent?
			if (underfilled->parent->at_least_half_full())
			{
				assert(isValid());
				return true;
			}
			underfilled = underfilled->parent;
		}
		//check left
		else if (leftSibling &&
			underfilled->getNumValues() + leftSibling->getNumValues() < BTREE_FANOUT - 1)
		{	//MERGE left
			out = leftSibling->merge(underfilled, underfilled_idx - 1);
			//fix parent
			cout << "before remove child line 404" << endl;
			leftSibling->parent->remove_child(underfilled_idx);
			cout << "after remove child line 404" << endl;
			leftSibling->parent->remove_value(underfilled_idx - 1);
			//did we underfill the parent?
			if (leftSibling->parent->at_least_half_full())
			{
				assert(isValid());
				return true;
			}
			underfilled = leftSibling->parent;
		}

		//continue up tree
	} //LOOP
	  //if we ever reach this code remind me to jump off a bridge after class
	assert(false);
	return false;
}

vector<Data*> Btree::search_range(VALUETYPE begin, VALUETYPE end) 
{
    // TODO: Implement this
	assert(root);
	vector<Data*> v;

	//find node where first value would appear
	Bnode* current = root;
	Bnode_inner* inner = dynamic_cast<Bnode_inner*>(current);
	while (inner)		//while current isn't a leaf...
	{
		//which child should have this new value?
		int child_idx = inner->find_value_gt(begin);
		current = inner->getChild(child_idx);
		inner = dynamic_cast<Bnode_inner*>(current);
	}
	Bnode_leaf* first_leaf = dynamic_cast<Bnode_leaf*>(current);
	assert(first_leaf);

	//find node where last value would appear
	current = root;
	inner = dynamic_cast<Bnode_inner*>(current);
	while (inner)		//while current isn't a leaf...
	{
		//which child should have this new value?
		int child_idx = inner->find_value_gt(end);
		current = inner->getChild(child_idx);
		inner = dynamic_cast<Bnode_inner*>(current);
	}
	Bnode_leaf* last_leaf = dynamic_cast<Bnode_leaf*>(current);
	assert(last_leaf);

	Bnode_leaf* focus = first_leaf;
	while (focus)
	{
		for (int i = 0; i < focus->getNumValues(); ++i)
		{
			if (focus->get(i) < begin) continue;
			if (focus->get(i) > end) break;
			v.push_back(focus->getData(i));
		}
		
		if (focus == last_leaf) break;
		focus = focus->next;
	}

    return v;
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

