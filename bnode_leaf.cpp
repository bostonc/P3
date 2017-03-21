#include "bnode_leaf.h"
#include <vector>

using namespace std;

Bnode_leaf::~Bnode_leaf() {
//     // Remember to deallocate memory!!
// 	for (int i = 0; i < num_values; ++i)
// 	{
// 		delete values[i];
// 	}	
// 	//delete[] values;
}

VALUETYPE Bnode_leaf::merge(Bnode_leaf* rhs) {
    assert(num_values + rhs->getNumValues() < BTREE_LEAF_SIZE);
    assert(rhs->num_values > 0);
    VALUETYPE retVal = rhs->get(0);

    Bnode_leaf* save = next;
    next = next->next;
    if (next) next->prev = this;

    for (int i = 0; i < rhs->getNumValues(); ++i)
        insert(rhs->getData(i));

    rhs->clear();
    return retVal;
}

VALUETYPE Bnode_leaf::redistribute(Bnode_leaf* rhs) {
    // TODO: Implement this
	
// 	//make vector of all values
// 	vector<VALUETYPE> all_values(values, values + num_values);
	
// 	//add rhs values to the vector
// 	int num_vals = rhs->getNumValues();
// 	for (int i = 0; i < num_vals; i++) {
// 		all_values.push_back(rhs->get(i));
// 	}
	
// 	int total_vals = all_values.size();
	
// 	//populate this with first half of values
// 	for (int i = 0; i < total_vals / 2; i++) {
// 		insert(all_values[i]);
// 	}
	
// 	//populate rhs with second half of values
// 	for (int i = total_vals / 2; i < total_vals; i++) {
// 		rhs->insert(all_values[i]);
// 	}
	
// 	//smallest value in rhs should be returned as what's going to be the new value of the parent
// 	VALUETYPE new_parent_val = all_values[total_vals / 2];
	
// 	//add asserts?
	
	
//     return new_parent_val;
	return -1;

}

Bnode_leaf* Bnode_leaf::split(VALUETYPE insert_value) {
//     assert(num_values == BTREE_LEAF_SIZE); //only split when full

// 	//populate temp array with all values before splitting
// 	vector<VALUETYPE> all_values(values, values + num_values);

// 	//create new node
// 	Bnode_leaf* split_node = new Bnode_leaf;

// 	//give first half of values to this leaf
// 	clear();
// 	for (int i = 0; i < BTREE_LEAF_SIZE / 2; ++i)
// 	{
// 		insert(all_values[i]);
// 	}

// 	//give second half of values to the new leaf
// 	for (int i = (BTREE_LEAF_SIZE / 2); i < BTREE_LEAF_SIZE; ++i)
// 	{
// 		split_node->insert(all_values[i]);
// 	}

// 	//insert new value
// 		//if both nodes are valid, put in left
// 	if (insert_value > all_values[BTREE_LEAF_SIZE / 2 - 1] &&
// 		insert_value < all_values[BTREE_LEAF_SIZE / 2])
// 	{
// 		insert(insert_value);
// 		assert(num_values == BTREE_LEAF_SIZE / 2 + 1);
// 		assert(split_node->num_values == BTREE_LEAF_SIZE / 2);
// 	}
// 		//if right is only valid
// 	else if (insert_value > all_values[BTREE_LEAF_SIZE / 2])
// 	{
// 		split_node->insert(insert_value);
// 		assert(num_values == BTREE_LEAF_SIZE / 2);
// 		assert(split_node->num_values == BTREE_LEAF_SIZE / 2 + 1);
// 	}
// 		//else, left is only valid
// 	else
// 	{
// 		insert(insert_value);
// 		assert(num_values == BTREE_LEAF_SIZE / 2 + 1);
// 		assert(split_node->num_values == BTREE_LEAF_SIZE / 2);
// 	}	

// 	//assign parent & siblings
// 	next->prev = split_node;
// 	split_node->next = next;

// 	next = split_node;
// 	split_node->prev = this;	

// 	split_node->parent = parent;
	
// 	//use first value of split node as new parent data
//     return split_node;
}


