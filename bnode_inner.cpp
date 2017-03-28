#include "bnode_inner.h"
#include <vector>
#include <iostream> //for debugging

using namespace std;

Bnode_inner* Bnode_inner::common_ancestor(Bnode* rhs)
{
	Bnode_inner* leftParent = parent;
	Bnode_inner* rightParent = rhs->parent;

	if (leftParent == nullptr || rightParent == nullptr) return nullptr;

	while (leftParent != rightParent)
	{
		leftParent = leftParent->parent;
		rightParent = rightParent->parent;
	}
	assert(leftParent == rightParent);
	return leftParent;
}

VALUETYPE Bnode_inner::merge(Bnode_inner* rhs, int parent_idx) {
    assert(rhs->parent == parent); // can only merge siblings (given)
    //assert(rhs->num_values > 0); //given
	assert(num_values + rhs->getNumValues() < BTREE_FANOUT); //may not be correct
	assert(num_children + rhs->getNumChildren() <= BTREE_FANOUT); //may not be correct
	
	//move all values to lhs, including appropriate parent value
	for (int i = 0; i < rhs->getNumValues(); ++i)
	{
		insert(rhs->get(i));
	}
	insert(parent->get(parent_idx));

	//move all children to lhs
	for (int i = 0; i < rhs->getNumChildren(); ++i)
	{
		insert(rhs->getChild(i), num_children + i);
	}
	

	//clear rhs, replace parent value
	rhs->clear();
	VALUETYPE highest_val = values[0];
	for (int i = 0; i < num_values; ++i)
	{
		if (values[i] > highest_val) highest_val = values[i];
	}
	//parent->insert(highest_val);

	//returns value which was written to parent
	return highest_val;
}

VALUETYPE Bnode_inner::redistribute(Bnode_inner* rhs, int parent_idx) {
    assert(rhs->parent == parent); // inner node redistribution should only happen with siblings
    assert(parent_idx >= 0);
    assert(parent_idx < parent->getNumValues());

    // TODO: Implement this
    
    //make a vector or all values and a vector of all children in this node
    vector<VALUETYPE> all_values(values, values + num_values);
    vector<Bnode*> all_children(children, children + num_children);
	
	//add value from parent through which redistribution occurs if necessary
	//(makes rotations work correctly)
	bool rotating = false;
	if (parent->get(parent_idx) != rhs->get(0))
	{
		all_values.push_back(parent->get(parent_idx));
		rotating = true;
	}		
    
    //add the values and children of rhs to the vector
    int num_vals = rhs->getNumValues();
    int num_child = rhs->getNumChildren();
    
    for (int i = 0; i < num_vals; i++) {
        all_values.push_back(rhs->get(i));
        
    }
	for (int i = 0; i < num_vals + 1; i++) {
		all_children.push_back(rhs->getChild(i));
	}
		
	
    int total_vals = all_values.size();
    int total_children = all_children.size();
	if (rotating)
	{
		assert(total_vals == num_values + rhs->getNumValues() + 1);
		assert(total_vals <= BTREE_FANOUT * 2 - 1);
	}
	else
	{
		assert(total_vals == num_values + rhs->getNumValues());
		assert(total_vals < BTREE_FANOUT * 2 - 1);		
	}
	
	assert(total_children == num_children + rhs->getNumChildren());
	assert(total_children <= BTREE_FANOUT * 2);	
    

	VALUETYPE new_parent_val = -1;
	clear();
	rhs->clear();
	if (rotating) //if rotating
	{
		
		for (int i = 0; i < (total_vals - 1) / 2; i++) {
			insert(values[i]);
		}
		for (int i = 0; i < ((total_vals - 1) / 2) + 1; i++) {
			insert(children[i], i);
		}
		parent->replace_value(values[(total_vals - 1) / 2], parent_idx);
		for (int i = ((total_vals - 1) / 2) + 1; i < total_vals; i++) {
			rhs->insert(values[i]);
		}
		for (int i = ((total_vals - 1) / 2) + 1, index = 0; i < total_vals + 1; i++, index++) {
			rhs->insert(children[i], index);
		}
		
		
		
		
		
		
		//shift
// 		int count = 0;
// 		while (true)
// 		{

			
// 			rhs->insert(parent->get(parent_idx));
// 			rhs->insert(children[num_children - 1], 0);
// 			parent->replace_value(values[num_values - 1], parent_idx);
// 			remove_value(num_values - 1);
// 			remove_child(num_children - 1);


// 		}

	}
	else //not rotating...
	{

		//populate this with first half of values
		for (int i = 0; i < total_vals / 2; i++) {
			insert(all_values[i]);
		}
		for (int i = 0, idx = 0; i < total_vals / 2 + 1; i++, idx++) {
			insert(all_children[i], idx);
			all_children[i]->parent = this;
		}

		//middle value should be returned as what's going to be the value of the parent
		new_parent_val = all_values[total_vals / 2];

		//populate rhs with second half of values
		for (int i = total_vals / 2 + 1; i < total_vals + 1; i++) {
			rhs->insert(all_values[i]);
		}
		for (int i = total_vals / 2 + 1, idx = 0; i < total_children; i++, idx++) {
			rhs->insert(all_children[i], idx);
			all_children[i]->parent = rhs;
		}

	}



	

    
    
// 	assert(total_vals == num_values + rhs->getNumValues());
// 	assert(num_values == rhs->getNumValues() || rhs->getNumValues() == num_values + 1);
// 	assert(num_values < BTREE_FANOUT && num_values >= BTREE_FANOUT / 2);
// 	assert(rhs->getNumValues() < BTREE_FANOUT && rhs->getNumValues() >= BTREE_FANOUT / 2);
// 	assert(total_children == num_children + rhs->getNumChildren());
// 	assert(num_children == rhs->getNumChildren() || rhs->getNumChildren() == num_children + 1);
// 	assert(num_children <= BTREE_FANOUT && num_children >= BTREE_FANOUT / 2);
// 	assert(rhs->getNumChildren() <= BTREE_FANOUT && rhs->getNumChildren() >= BTREE_FANOUT / 2);
	
    
    

    return new_parent_val;
}


Bnode_inner* Bnode_inner::split(VALUETYPE& output_val, VALUETYPE insert_value, Bnode* insert_node) {
    assert(num_values == BTREE_FANOUT-1); // only split when it's full!

    // Populate an intermediate array with all the values/children before splitting - makes this simpler
    vector<VALUETYPE> all_values(values, values + num_values);
    vector<Bnode*> all_children(children, children + num_children);

    // Insert the value that created the split
    int ins_idx = find_value_gt(insert_value);
    all_values.insert(all_values.begin()+ins_idx, insert_value);
    all_children.insert(all_children.begin()+ins_idx+1, insert_node);

    // Do the actual split into another node
    Bnode_inner* split_node = new Bnode_inner;

    assert(all_values.size() == BTREE_FANOUT);
    assert(all_children.size() == BTREE_FANOUT+1);

    // Give the first BTREE_FANOUT/2 values to this bnode
    clear();
    for (int i = 0; i < BTREE_FANOUT/2; ++i)
        insert(all_values[i]);
    for (int i = 0, idx = 0; i < (BTREE_FANOUT/2) + 1; ++i, ++idx) {
        insert(all_children[i], idx);
        all_children[i] -> parent = this;
    }

    // Middle value should be pushed to parent
    output_val = all_values[BTREE_FANOUT/2];

    // Give the last BTREE/2 values to the new bnode
    for (int i = (BTREE_FANOUT/2) + 1; i < (int)all_values.size(); ++i)
        split_node->insert(all_values[i]);
    for (int i = (BTREE_FANOUT/2) + 1, idx = 0; i < (int)all_children.size(); ++i, ++idx) {
        split_node->insert(all_children[i], idx);
        all_children[i] -> parent = split_node;
    }

    // I like to do the asserts :)
    assert(num_values == BTREE_FANOUT/2);
    assert(num_children == num_values+1);
    assert(split_node->getNumValues() == BTREE_FANOUT/2);
    assert(split_node->getNumChildren() == num_values + 1);

    split_node->parent = parent; // they are siblings

    return split_node;
}
