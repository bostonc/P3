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

bool Btree::remove(VALUETYPE value) {
	//cout << "in remove, value = " << value << endl;
	assert(root);
	
	//if the value's not in the tree, return false
	if (search(value) == nullptr) return false;
	
	//else find node with the value
	Bnode* current = root;
	Bnode_inner* inner = dynamic_cast<Bnode_inner*>(current);
	
	//while current isn't a leaf
	while (inner) {	
		//which child should have this new value?
		int child_idx = inner->find_value_gt(value);
		//cout << "child_idx = " << child_idx << endl;
		current = inner->getChild(child_idx);
		inner = dynamic_cast<Bnode_inner*>(current);

	}
	//found the leaf node where the value is
	Bnode_leaf* leaf = dynamic_cast<Bnode_leaf*>(current);
	assert(leaf);
	
	//remove value and decrease size
	leaf->remove(value);
	size--;
	
	bool done = false;
	//if that leaf is half of more full, done = true
	if (leaf->at_least_half_full() && leaf->getNumValues() <= BTREE_LEAF_SIZE) {
		done = true;
	}
	
	//else
	else {
		//if right leaf node exists and is more than half full
		if (leaf->next && leaf->next->getNumValues() > BTREE_LEAF_SIZE / 2) {
			//redistribute and set parent val to closest ancestor and set done = true
			Bnode_inner* common_ansc = leaf->common_ancestor(leaf->next);
			//get leaf's highest value
			VALUETYPE leaf_high;
			if (leaf->getNumValues() == 0) {
				leaf_high = value;
			}
			else {
				leaf_high = leaf->get(leaf->getNumValues() - 1);
			}
			//get leaf->next's lowest value
			VALUETYPE leaf_next_low = leaf->next->get(0);
			//get index of common ancsestor
			int index = 0;
			for (int i = 0; i < common_ansc->getNumValues(); i++) {
				if (common_ansc->get(i) > leaf_high && common_ansc->get(i) <= leaf_next_low) {
					index = i;
				}
			}
			//cout << "before redistribute\n";
			//debugging
			for (int i = 0; i < leaf->getNumValues(); i++) {
				//cout << "value: " << leaf->get(i) << endl;
			}
			for (int i = 0; i < leaf->next->getNumValues(); i++) {
				//cout << "value: " << leaf->next->get(i) << endl;
			}
			VALUETYPE new_parent_val = leaf->redistribute(leaf->next);
			//cout << "new_parent_val: " << new_parent_val << endl;
			common_ansc->replace_value(new_parent_val, index);
			done = true;
			//cout << "done with redistribution\n";
			
		}
		//else if left leaf node exists and is more than half full
		else if (leaf->prev && leaf->prev->getNumValues() > BTREE_LEAF_SIZE / 2) {
			//redistribute and set parent val to closest ancestor and set done = true
			Bnode_inner* common_ansc = leaf->prev->common_ancestor(leaf);
			//get leaf->prev's highest value
			VALUETYPE leaf_prev_high = leaf->prev->get(leaf->prev->getNumValues() - 1);
			//get leaf's lowest value
			VALUETYPE leaf_low;
			if (leaf->getNumValues() == 0) {
				leaf_low = value;
			}
			else {
				leaf_low = leaf->get(0);
			}
			//get index of common ancsestor
			int index = 0;
			for (int i = 0; i < common_ansc->getNumValues(); i++) {
				if (common_ansc->get(i) > leaf_prev_high && common_ansc->get(i) <= leaf_low) {
					index = i;
				}
			}
			VALUETYPE new_parent_val = leaf->prev->redistribute(leaf);
			common_ansc->replace_value(new_parent_val, index);
			done = true;
		}
		//else
		//KINDA CONFUSED ABOUT THE VALUE MERGE RETURNS
		
		else {
			Bnode_inner* check_node = nullptr;
			cout << "in merge" << endl;
			Bnode_inner* common_ansc = nullptr;
			//if right leaf node exits
			if (leaf->next) {
				//cout << "in merge with leaf->next\n";
				//merge and set parent val to closest ancestor
				if (leaf->parent == leaf->next->parent) {
					//cout << "same parent" << endl;
					common_ansc = leaf->parent;
				}
				else {
					common_ansc = leaf->common_ancestor(leaf->next);
				}
				int index = 0;
				for (int i = 0; i < leaf->next->parent->getNumChildren(); i++) {
					if (leaf->next->parent->getChild(i) == leaf->next) {
						index = i;
					}
				}
				cout << "before merge" << endl;
				if (leaf->next) {
					cout << "yes leaf->next" << endl;
				}
				VALUETYPE to_remove_upper = leaf->merge(leaf->next);
				cout << "after merge, index = " << index << endl;
				if (leaf->next) {
					cout << "yes leaf->next" << endl;
				}
				if (!leaf->next) {
					cout << "no leaf->next" << endl;
				}
				if (leaf->next) {
					leaf->next->parent->remove_child(index); //should be leaf->next->parent
					if (leaf->next->parent->getNumChildren() < 2 && leaf->next->parent->parent) {
						check_node = leaf->next->parent;
					}
				}
				else {
					leaf->parent->remove_child(index); //don't think this is right
				}
				cout << "removed child" << endl;
				//cout << "to_remove_upper: " << to_remove_upper << endl;
				
				VALUETYPE to_remove_lower = value;
				cout << "before for loop" << endl;
				for (int i = 1; i < leaf->getNumValues(); i++) {
					if (leaf->get(i) == to_remove_upper) {
						to_remove_lower = leaf->get(i - 1);
					}
				}
				cout << "set remove upper and lower" << endl;
					//cout << "to_remove_lower: " << to_remove_lower << endl;
					//assuming that the value merge returns should be found in closest ancestor's node and removed
				
				
					//cout << "found common ancestor" << endl;
				for (int i = 0; i < common_ansc->getNumValues(); i++) {
					if (common_ansc->get(i) <= to_remove_upper && common_ansc->get(i) > to_remove_lower) {
						common_ansc->remove_value(i);
					}
				}
				cout << "done merging" << endl;
			
				
				//cout << "removed value" << endl;
			}
			//else if left leaf node exists
			else if (leaf->prev) {
				//merge and set parent val to closest ancestor
				//merge and set parent val to closest ancestor
				if (leaf->prev->parent == leaf->parent) {
					//cout << "same parent" << endl;
					common_ansc = leaf->prev->parent;
				}
				else {
					common_ansc = leaf->prev->common_ancestor(leaf);
				}
				int index = 0;
				for (int i = 0; i < leaf->prev->parent->getNumChildren(); i++) {
					if (leaf->prev->parent->getChild(i) == leaf) {
						index = i;
					}
				}
				
				if (leaf->getNumValues() > 0) {
					VALUETYPE to_remove_upper = leaf->prev->merge(leaf);
					leaf->prev->parent->remove_child(index);
					VALUETYPE to_remove_lower = value;
					for (int i = 1; i < leaf->prev->getNumValues(); i++) {
						if (leaf->prev->get(i) == to_remove_upper) {
							to_remove_lower = leaf->prev->get(i - 1);
						}
					}
				
					    
					//assuming that the value merge returns should be found in closest ancestor's node and removed
					//common_ansc = leaf->common_ancestor(leaf->next);
					for (int i = 0; i < common_ansc->getNumValues(); i++) {
						if (common_ansc->get(i) <= to_remove_upper && common_ansc->get(i) > to_remove_lower) {
							common_ansc->remove_value(i);
						}
					}
				}
				else {
					leaf->prev->parent->remove_child(index);
					VALUETYPE to_remove_lower = leaf->prev->get(leaf->prev->getNumValues() - 1);
					for (int i = 0; i < common_ansc->getNumValues(); i++) {
						if (common_ansc->get(i) > to_remove_lower) {
							common_ansc->remove_value(i);
						}
					}
				}
					
				
				//set leaf = leaf->prev (I think?)
				leaf = leaf->prev;
			}
			//else
			else {
				return false; //shouldn't happen, return false?
			}
			bool fixed = false;
			//if parent inner node is at least half full
			//should this check common ancestor instead of parent?
			//cout << "still here" << endl;
			cout << "fixed set to false" << endl;
			Bnode_inner* node = nullptr;
			if (check_node) {
				node = check_node;
			}
			else {
				if (common_ansc->getNumValues() >= (BTREE_FANOUT - 1) / 2) { //make sure merge handles pointers right
					//fixed = true
					fixed = true;
					cout << "in if, fixed = true" << endl;
				
				}
				//cout << "how about now?" << endl;
				//set temp variables
				Bnode_inner* node = common_ansc;
				cout << "node set" << endl;
			}
			
			//cout << "and now?" << endl;
				
			//while !fixed
			while (!fixed) {
				cout << "in while loop" << endl;
				if (check_node) {
					cout << "check node is on" << endl;
				}
				//cout << "in while loop" << endl;
				//set temp variables 
				cout << "still working?" << endl;
				Bnode_inner* node_parent = node->parent;
				cout << "how about now?" << endl;
				if (node_parent == nullptr) {
					cout << "in if" << endl;
					//cout << "num node vals: " << node->getNumValues() << endl;
					//cout << "num node children: " << node->getNumChildren() << endl;
					if (node->getNumValues() == 0) {
						if (node->getNumChildren() == 0) {
							fixed = true;
							break;
						}
						else if (node->getNumChildren() == 1) {
							cout << "in else if" << endl;
							root = node->getChild(0);
							cout << "made new root" << endl;
							fixed = true;
							cout << "fixed" << endl;
							break;
						}
					}
					else {
						fixed = true;
						break;
					}
				}
				int node_idx = 0;
				for (int i = 0; i < node_parent->getNumChildren(); i++) {
					if (node_parent->getChild(i) == node) {
					node_idx = i;
					}
				
				}
				Bnode_inner* node_prev = nullptr;
				Bnode_inner* node_next = nullptr;
				if (node_idx > 0) {
					node_prev = dynamic_cast<Bnode_inner*>(node_parent->getChild(node_idx - 1));
				}
				if (node_idx < node_parent->getNumChildren() - 1) {
					node_next = dynamic_cast<Bnode_inner*>(node_parent->getChild(node_idx + 1));
				}
				
				//if right sibling inner node exists and is more than half full
				if (node_next && node_next->getNumValues() > (BTREE_FANOUT - 1) / 2) {
					//redistribute including parent and set new parent val to parent
					VALUETYPE parent_val = node->redistribute(node_next, node_idx);
					node_parent->replace_value(parent_val, node_idx);
					fixed = true;
					
					
				}
				//else if left sibling inner node exists and is more than half full
				else if (node_prev && node_prev->getNumValues() > (BTREE_FANOUT - 1) / 2) {
					//redistribute including parent val and set new parent val to parent
					VALUETYPE parent_val = node_prev->redistribute(node, node_idx - 1);
					node_parent->replace_value(parent_val, node_idx - 1);
					fixed = true;
					
				}
				//else
				else {
					//if right sibling inner node exists
					if (node_next) {
						//merge including parent and remove parent_val from parent
						VALUETYPE parent_val = node->merge(node_next, node_idx);
						node_parent->remove_value(node_idx);
						
						
					}
					//else if left sibling inner node exists
					else if (node_prev) {
						//merge including parent and remove parent_val from parent
						VALUETYPE parent_val = node_prev->merge(node, node_idx - 1);
						node_parent->remove_value(node_idx - 1);
						
					}
					//else 
					else {
						//it's a root node, return true
						return true;
	
					}
					node = node_parent;
					//if grandparent node doesn't exist or is half full or more
					if (node->getNumValues() >= (BTREE_FANOUT - 1) / 2) {
						//fixed = true
						fixed = true;
					}
				}
			}
			//if fixed, done = true
			if (fixed) {
				done = true;
			}
		}
	}
	//if done
	if (done) {
		//cout << "in done" << endl;
		//check asserts and return true
		assert(isValid());
		assert(leaf->getNumValues() >= BTREE_LEAF_SIZE / 2 && leaf->getNumValues() <= BTREE_LEAF_SIZE);
		//cout << "returning" << endl;
		return true;
	}
	//shouldn't get here
	return false;
}
	
	

	
   

bool Btree::remove_chris(VALUETYPE value)
{
	assert(root);

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
		int idx = ancestor->find_value_gt(out) - 1; //OFF BY 1??????????
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
		out = leaf->merge(leaf->next);
		//fix parent of right node, and commmon ancestor if different
		rightParent->remove_child(idx); //MAKE SURE NOT TO DO THIS IN MERGE. MEMORY LEAK?????
										//if we merged siblings...
		if (leaf->parent == rightParent) rightParent->remove_value(idx - 1);
		//if we merged non-siblings...
		if (leaf->parent != rightParent)
		{
			rightParent->remove_value(0);
			//fix value of common ancestor
			idx = leaf->parent->common_ancestor(rightParent)->find_value_gt(out) - 1;
			leaf->parent->common_ancestor(rightParent)->replace_value(out, idx);
		}
		//see if rightParent is underfilled now, return if we're good
		if (rightParent->at_least_half_full()) return true;
		underfilled == rightParent;
	}
	//else, we merge with the left
	else if (leaf->prev)
	{
		Bnode_inner* rightParent = leaf->parent;
		int idx = rightParent->find_child(leaf);
		out = leaf->prev->merge(leaf);
		//fix parent of right node, and common ancestor if different
		rightParent->remove_child(idx); //MAKE SURE NOT TO DO THIS IN MERGE. MEMORY LEAK?????
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
		if (rightParent->at_least_half_full()) return true;
		underfilled == rightParent;
	}

	//damn, something higher up must be underfilled...

	//LOOP

	//root check (is parent underfilled? if yes && parent==root, reduce height)
	//redist (only siblings now)
	//merge (only siblings now)






	//if we can't redistribute or merge, we must be at the root

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

