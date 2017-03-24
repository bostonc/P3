#include "btree.h"
#include "bnode.h"
#include "bnode_inner.h"
#include "bnode_leaf.h"
#include <vector>
#include <cassert>

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
    // TODO: Implement this
	
	assert(root);
    Bnode* current = root;

    // Have not reached a leaf node yet
    Bnode_inner* inner = dynamic_cast<Bnode_inner*>(current);
    
    while (inner) {
        int find_index = inner->find_value_gt(value);
        current = inner->getChild(find_index);
        inner = dynamic_cast<Bnode_inner*>(current);
    }

    // Found a leaf node
    Bnode_leaf* leaf = dynamic_cast<Bnode_leaf*>(current);
    assert(leaf);
    for (int i = 0; i < leaf->getNumValues(); ++i) {
        if (leaf->get(i) > value)    return false; // passed the possible location
        
	//we found the value to remove
	if (leaf->get(i) == value) {
		//remove data entry
		if (!remove(leaf->get(i))) {
			//not removed successfully
			return false; //?
		}
		size--;
		
		//check if the leaf node is less than half full
		if (leaf->getNumValues() < BTREE_LEAF_SIZE / 2) {
			//check if there's a node we can redistribute with
			if (leaf->next && leaf->next->getNumValues() > BTREE_LEAF_SIZE / 2) {
				//redistribute with right node (next)
				VALUETYPE new_parent_val = leaf->redistribute(leaf->next);
				if (leaf->parent == leaf->next->parent) {
					leaf->parent->insert(new_parent_val);
				}
				else {
					//find common ancestor
					Bnode_inner* temp_leaf1 = dynamic_cast<Bnode_inner*>(current);
					Bnode_inner* temp_leaf2 = dynamic_cast<Bnode_inner*>(current);
					temp_leaf1 = leaf->parent;
					temp_leaf2 = leaf->next->parent;
					bool found = false;
					while(!found || temp_leaf1 != root) {
						if (temp_leaf1->parent == temp_leaf2->parent) {
							temp_leaf1->parent->insert(new_parent_val);
							found = true;
						}
						else {
							temp_leaf1 = temp_leaf1->parent;
							temp_leaf2 = temp_leaf2->parent;
						}
					}
				}
				//I think we're done
				assert(isValid());
				assert(leaf->getNumValues() >= BTREE_LEAF_SIZE / 2 && leaf->getNumValues() < BTREE_LEAF_SIZE);
				return true;
				
			}
			else if (leaf->prev && leaf->getNumValues() > BTREE_LEAF_SIZE / 2) {
				//redistribute with left node (prev)
				VALUETYPE new_parent_val = leaf->prev->redistribute(leaf);
				if (leaf->parent == leaf->prev->parent) {
					leaf->parent->insert(new_parent_val);
				}
				else {
					//find common ancestor
					Bnode_inner* temp_leaf1 = dynamic_cast<Bnode_inner*>(current);
					Bnode_inner* temp_leaf2 = dynamic_cast<Bnode_inner*>(current);
					temp_leaf1 = leaf->parent;
					temp_leaf2 = leaf->prev->parent;
					bool found = false;
					while(!found || temp_leaf1 != root) {
						if (temp_leaf1->parent == temp_leaf2->parent) {
							temp_leaf1->parent->insert(new_parent_val);
							found = true;
						}
						else {
							temp_leaf1 = temp_leaf1->parent;
							temp_leaf2 = temp_leaf2->parent;
						}
					}
				}
				//I think we're done
				assert(isValid());
				assert(leaf->getNumValues() >= BTREE_LEAF_SIZE / 2 && leaf->getNumValues() < BTREE_LEAF_SIZE);
				return true;
				
			}
			//if not, merge with a node
			else if (leaf->next) {
				//merge with right node (next)
				VALUETYPE new_parent_val = leaf->merge(leaf->next);
				if (leaf->parent == leaf->next->parent) {
					
					leaf->parent->insert(new_parent_val); //not sure about this for merge
				}
				else {
					//find common ancestor
					Bnode_inner* temp_leaf1 = dynamic_cast<Bnode_inner*>(current);
					Bnode_inner* temp_leaf2 = dynamic_cast<Bnode_inner*>(current);
					temp_leaf1 = leaf->parent;
					temp_leaf2 = leaf->next->parent;
					bool found = false;
					while(!found || temp_leaf1 != root) {
						if (temp_leaf1->parent == temp_leaf2->parent) {
							temp_leaf1->parent->insert(new_parent_val); //not sure about this for merge
							found = true;
						}
						else {
							temp_leaf1 = temp_leaf1->parent;
							temp_leaf2 = temp_leaf2->parent;
						}
					}
				}
				//fix tree
				if (leaf->parent->getNumChildren() > BTREE_FANOUT / 2) {
					//we're done
					assert(isValid());
					assert(leaf->getNumValues() >= BTREE_LEAF_SIZE / 2 && leaf->getNumValues() < BTREE_LEAF_SIZE);
					return true;
				}
				else {
					//do another merge/fix something
					//while loop to fix it
					bool fixed = false;
					Bnode_inner* temp1 = dynamic_cast<Bnode_inner*>(current);
						Bnode_inner* temp2 = dynamic_cast<Bnode_inner*>(current);
						Bnode_inner* temp3 = dynamic_cast<Bnode_inner*>(current);
						temp1 = leaf->parent;
						temp2 = leaf->parent->next;
						temp3 = leaf->parent->prev;
					while (!fixed) {
						
						if (temp2 && temp2->parent == temp1->parent) {
							//they're siblings, merge
							VALUETYPE parent_val = temp1->merge(temp2);
							temp1->parent->insert(parent_val); //not sure about this for merge
							if (temp1->parent->getNumChildren() > BTREE_FANOUT / 2) {
								fixed = true;
							}
							else {
								temp1 = temp1->parent;
								temp2 = temp1->parent->next;
								temp3 = temp1->parent->prev;
							}
							
						}
						else if (temp3 && temp3->parent == temp1->parent) {
							//they're siblings, merge
							VALUETYPE parent_val = temp3->merge(temp1);
							temp3->parent->insert(parent_val); //not sure about this for merge
							if (temp3->parent->getNumChildren() > BTREE_FANOUT / 2) {
								fixed = true;
							}
							else {
								temp1 = temp3->parent;
								temp2 = temp3->parent->next;
								temp3 = temp3->parent->prev;
							}
							
						}
						else {
							//no siblings, rotate
						}
					}
					//assert stuff, then
					assert(isValid());
					assert(leaf->getNumValues() >= BTREE_LEAF_SIZE / 2 && leaf->getNumValues() < BTREE_LEAF_SIZE);
					return true;
				}
				
			}
			else if (leaf->prev) {
				//merge with left node (prev)
				VALUETYPE new_parent_val = leaf->prev->merge(leaf);
				if (leaf->parent == leaf->prev->parent) {
					leaf->parent->insert(new_parent_val);
				}
				else {
					//find common ancestor
					Bnode_inner* temp_leaf1 = dynamic_cast<Bnode_inner*>(current);
					Bnode_inner* temp_leaf2 = dynamic_cast<Bnode_inner*>(current);
					temp_leaf1 = leaf->parent;
					temp_leaf2 = leaf->prev->parent;
					bool found = false;
					while(!found || temp_leaf1 != root) {
						if (temp_leaf1->parent == temp_leaf2->parent) {
							temp_leaf1->parent->insert(new_parent_val); //not sure about this for merge
							found = true;
						}
						else {
							temp_leaf1 = temp_leaf1->parent;
							temp_leaf2 = temp_leaf2->parent;
						}
					}
				}
				//fix tree
				if (leaf->parent->getNumChildren() > BTREE_FANOUT / 2) {
					//we're done
					assert(isValid());
					assert(leaf->getNumValues() >= BTREE_LEAF_SIZE / 2 && leaf->getNumValues() < BTREE_LEAF_SIZE);
					return true;
				}
				else {
					//do another merge/fix something
					//while loop to fix it
					bool fixed = false;
					Bnode_inner* temp1 = dynamic_cast<Bnode_inner*>(current);
						Bnode_inner* temp2 = dynamic_cast<Bnode_inner*>(current);
						Bnode_inner* temp3 = dynamic_cast<Bnode_inner*>(current);
						temp1 = leaf->parent;
						temp2 = leaf->parent->next;
						temp3 = leaf->parent->prev;
					while (!fixed) {
						
						if (temp2 && temp2->parent == temp1->parent) {
							//they're siblings, merge
							VALUETYPE parent_val = temp1->merge(temp2);
							temp1->parent->insert(parent_val); //not sure about this for merge
							if (temp1->parent->getNumChildren() > BTREE_FANOUT / 2) {
								fixed = true;
							}
							else {
								temp1 = temp1->parent;
								temp2 = temp1->parent->next;
								temp3 = temp1->parent->prev;
							}
							
						}
						else if (temp3 && temp3->parent == temp1->parent) {
							//they're siblings, merge
							VALUETYPE parent_val = temp3->merge(temp1);
							temp3->parent->insert(parent_val); //not sure about this for merge
							if (temp3->parent->getNumChildren() > BTREE_FANOUT / 2) {
								fixed = true;
							}
							else {
								temp1 = temp3->parent;
								temp2 = temp3->parent->next;
								temp3 = temp3->parent->prev;
							}
							
						}
						else {
							//no siblings, rotate
						}
					}
					//assert stuff, then
					assert(isValid());
					assert(leaf->getNumValues() >= BTREE_LEAF_SIZE / 2 && leaf->getNumValues() < BTREE_LEAF_SIZE);
					return true;
				}
			}
			else {
				//it's a root node?
				if (leaf->getNumValues() == 0) {
					delete leaf;
				}
				
			}
			
			//fix tree
			//return true when it's all done
			assert(isValid());
			assert(leaf->getNumValues() >= BTREE_LEAF_SIZE / 2 && leaf->getNumValues() < BTREE_LEAF_SIZE);
			       return true;
		}
		else {
			assert(isValid());
			assert(leaf->getNumValues() >= BTREE_LEAF_SIZE / 2 && leaf->getNumValues() < BTREE_LEAF_SIZE);
			return true; //once the node has been removed we're done if the leaf node is full enough
		}
		
	}
    }
	assert(isValid());
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

