#include "bnode_leaf.h"
#include "bnode_inner.h" //needed for common ancestor to work as member function
#include <vector>
#include <iostream> //for debugging

using namespace std;

Bnode_leaf::~Bnode_leaf() {
    // Remember to deallocate memory!!
	for (int i = 0; i < num_values; ++i)
	{
		delete values[i];
	}	
	//delete[] values;
}

Bnode_inner* Bnode_leaf::common_ancestor(Bnode* rhs)
{
	//cout << "in common ancestor\n";
	
	Bnode_inner* leftParent = parent;
	Bnode_inner* rightParent = rhs->parent;
	
	//cout << "assigned vars\n";
	
	//for debugging
	if (leftParent == rightParent) {
		//cout << "same parents\n";
	}

	if (leftParent == nullptr || rightParent == nullptr) return nullptr;

	while (leftParent != rightParent)
	{
		leftParent = leftParent->parent;
		rightParent = rightParent->parent;
	}
	assert(leftParent == rightParent);
	return leftParent;
}

VALUETYPE Bnode_leaf::merge(Bnode_leaf* rhs) {
	cout << "in merge" << endl;
    assert(num_values + rhs->getNumValues() < BTREE_LEAF_SIZE);
    //assert(rhs->num_values > 0);
    VALUETYPE retVal = rhs->get(0);

    Bnode_leaf* save = next;
    next = next->next;
    if (next) next->prev = this;

    for (int i = 0; i < rhs->getNumValues(); ++i) {
        insert(rhs->getData(i));
    }
	
	if (rhs) {
		cout << "yes rhs" << endl;
	}
    rhs->clear();
	if (rhs) {
		cout << "yes rhs" << endl;
	}
	cout << "return val: " << retVal << endl;
    return retVal;
}

VALUETYPE Bnode_leaf::redistribute(Bnode_leaf* rhs) {
    // TODO: Implement this
	//cout << "in leaf redistribute\n";
	
	//for debugging
// 	for (int i = 0; i < rhs->getNumValues(); i++) {
// 		cout << "value: " << rhs->get(i) << endl;
// 	}
	
	//make vector of all values
	vector<Data*> all_values(values, values + num_values);

	//add rhs values to the vector
	int num_vals = rhs->getNumValues();
	for (int i = 0; i < num_vals; i++) {
		//all_values.push_back(rhs->get(i));
		//cout << "value before: " << rhs->get(i) << endl;
		Data* temp = new Data(rhs->get(i));
		all_values.push_back(temp);
		//cout << "value after: " << all_values[i]->value << endl;
		//delete temp;
	}
	
	int total_vals = all_values.size();
	//cout << "total_vals: " << total_vals << endl;
	
	//debugging
// 	for (int i = 0; i < total_vals; i++) {
// 		cout << "value: " << all_values[i]->value << endl;
// 	}
	
	assert(total_vals == num_values + rhs->getNumValues());
	assert (total_vals <= BTREE_LEAF_SIZE * 2);
	
	clear();
	rhs->clear();
		
	//populate this with first half of values
	for (int i = 0; i < total_vals / 2; i++) {
		//cout << "inserting value: " << all_values[i]->value << endl;
		insert(all_values[i]);
	}
	
	//populate rhs with second half of values
	for (int i = total_vals / 2; i < total_vals; i++) {
		//cout << "inserting value: " << all_values[i]->value << endl;
		rhs->insert(all_values[i]);
	}
	
	//smallest value in rhs should be returned as what's going to be the new value of the parent
	Data* new_parent_val = all_values[total_vals / 2];
	//cout << "in redistribute new_parent_val->value: " << new_parent_val->value << endl;
	
	assert(total_vals == num_values + rhs->getNumValues());
	assert(num_values == rhs->getNumValues() || rhs->getNumValues() == num_values + 1);
	assert(num_values <= BTREE_LEAF_SIZE && num_values >= BTREE_LEAF_SIZE / 2);
	assert(rhs->getNumValues() <= BTREE_LEAF_SIZE && rhs->getNumValues() >= BTREE_LEAF_SIZE / 2);
	
	
	
    return new_parent_val->value;
}

Bnode_leaf* Bnode_leaf::split(VALUETYPE insert_value) {
    assert(num_values == BTREE_LEAF_SIZE); //only split when full

	//populate temp array with all values before splitting
	vector<Data*> all_values(values, values + num_values);

	//create new node
	Bnode_leaf* split_node = new Bnode_leaf;

	//give first half of values to this leaf
	clear();
	for (int i = 0; i < BTREE_LEAF_SIZE / 2; ++i)
	{
		insert(all_values[i]);
	}

	//give second half of values to the new leaf
	for (int i = (BTREE_LEAF_SIZE / 2); i < BTREE_LEAF_SIZE; ++i)
	{
		split_node->insert(all_values[i]);
	}

	//insert new value
		//if both nodes are valid, put in left
	if (insert_value > all_values[BTREE_LEAF_SIZE / 2 - 1]->value &&
		insert_value < all_values[BTREE_LEAF_SIZE / 2]->value)
	{
		insert(insert_value);
		assert(num_values == BTREE_LEAF_SIZE / 2 + 1);
		assert(split_node->num_values == BTREE_LEAF_SIZE / 2);
	}
		//if right is only valid
	else if (insert_value > all_values[BTREE_LEAF_SIZE / 2]->value)
	{
		split_node->insert(insert_value);
		assert(num_values == BTREE_LEAF_SIZE / 2);
		assert(split_node->num_values == BTREE_LEAF_SIZE / 2 + 1);
	}
		//else, left is only valid
	else
	{
		insert(insert_value);
		assert(num_values == BTREE_LEAF_SIZE / 2 + 1);
		assert(split_node->num_values == BTREE_LEAF_SIZE / 2);
	}	

	//assign parent & siblings
	if (next)
	{
		next->prev = split_node;
		split_node->next = next;
	}
	next = split_node;
	split_node->prev = this;	

	split_node->parent = parent;
	
	//use first value of split node as new parent data
    return split_node;
	//return nullptr;
}


