#include "btree.h"
#include "constants.h"
#include <iostream>
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <string> //added so first line of main() compiles.

using namespace std;


void small_test() {
    cout << "SMALL TEST: " << endl;
    Btree btree;
    cout << btree << endl;
    btree.insert(0);
    btree.insert(2);
    btree.insert(3);
    cout << btree << endl;
    btree.remove(0);
    cout << btree << endl;
    btree.remove(2);
    cout << btree << endl;
    btree.insert(2);
    cout << btree << endl;
    btree.insert(0);
    cout << btree << endl;
    btree.remove(2);
    cout << btree << endl;
    cout << "###########################" << endl;
    assert(btree.isValid());
}

//Test the spec example which is related to split
void splitTestFromSpec() {
    cout << "SPLIT TEST: " << endl;
    Btree btree;
    btree.insert(2); btree.insert(4);
    cout << btree << endl;
    btree.insert(5);
    cout << btree << endl;
    btree.remove(5); btree.remove(2); btree.insert(2);
    cout << btree << endl;
    btree.insert(3);
    cout << btree << endl;
    cout << "###########################" << endl;
    assert(btree.isValid());
}

//Test the spec example which is related to redistribution. You have to set BTREE_FANOUT to 3 and BTREE_LEAF_SIZE to 6
//in constants.h for testing the same case
void testForRedistribution() {
    cout << "REDISTRIBUTION TEST: " << endl;
    Btree btree;
    btree.insert(1); 
	cout << btree << endl;
	btree.insert(2); 
	cout << btree << endl;
	btree.insert(3); //with fanout = 3 and leaf size = 6, seg fault here
	cout << btree << endl;
	btree.insert(7); 
	cout << btree << endl;
	btree.insert(8);
	cout << btree << endl;
    btree.insert(10); btree.insert(11); btree.insert(9); btree.insert(12); btree.insert(13);
	cout << btree << endl;
    btree.insert(4);btree.insert(5);btree.insert(6);btree.insert(14);btree.insert(15);
    cout << btree << endl;
    
    btree.remove(9);
    cout << btree << endl;
    cout << "###########################" << endl;
    
    assert(btree.isValid());
}

//More comprehensive test for some edge cases. The last step("btree.remove(1)") tests the spec example which is related
//to merging
void large_test1() {
    cout << "LARGE TEST1: " << endl;
    Btree btree;
    btree.remove(0); //test to remove while btree is empty
    cout << btree << endl;
    btree.insert(1); btree.insert(2);//initial inserts, make sure the root is leaf at the beginning
    cout << btree << endl;
    btree.insert(1); //test to insert an existing value
    cout << btree << endl;
    btree.remove(3); //test to remove nonexistent value
    btree.remove(1); //test to remove from root
    cout << btree << endl;
    btree.insert(1);
    cout << btree << endl;
    btree.insert(3); //test an insertion which causes root-split
    cout << btree << endl;
    btree.remove(3); 
	cout << btree << endl;
	btree.remove(2);//test removals which makes the root leaf again 
    cout << btree << endl;
    btree.insert(3); btree.insert(5); btree.insert(7); btree.insert(9); btree.insert(11); //insertions which cause splits
    cout << btree << endl;
    btree.insert(2); btree.insert(4); btree.insert(6); btree.insert(8); btree.insert(10); //more insertions
    cout << btree << endl;
    btree.remove(11); //Prefer redistribution over merging for removals
    cout << btree << endl;
    btree.remove(5); 
	cout << btree << endl;
	btree.remove(6); //Prefer redistribution with right node over left node
    cout << btree << endl;
    btree.remove(8); //Prefer merging with right node over left node.
    cout << btree << endl;
    btree.remove(3); 
	btree.remove(2);
    cout << btree << endl;
    btree.remove(1);
    cout << btree << endl;
    cout << "###########################" << endl;
    
    assert(btree.isValid());
}

void large_test2() {
    cout << "LARGE TEST2: " << endl;
    Btree btree;
    btree.insert(1);
    btree.insert(2);
    btree.insert(3);
    btree.insert(4);
    btree.insert(6);
    btree.insert(8);
    btree.insert(13);
    btree.insert(15);
    btree.insert(17);
    cout << btree << endl;
    btree.insert(33);
    cout << btree << endl;
    btree.remove(13);
    cout << btree << endl;
    btree.remove(3); //BREAKS PARENT POINTERS
    cout << btree << endl;
    
    btree.insert(10);
    cout << btree << endl;
    btree.insert(7);
    
    cout << btree << endl;
    btree.remove(8);
    cout << btree << endl;
    
    btree.remove(6);
    cout << btree << endl;
    btree.insert(18);
    cout << btree << endl;
    btree.insert(19); //breaks things
    cout << btree << endl;
    btree.remove(17);
    cout << btree << endl;
    btree.insert(20);
    cout << btree << endl;
    btree.insert(34);
    cout << btree << endl;
    
    btree.insert(11);
    cout << btree << endl;
    btree.insert(12);
    cout << btree << endl;
    btree.insert(17);
    cout << btree << endl;
    
    btree.insert(13);
    cout << btree << endl;
    
    btree.remove(7);
    cout << btree << endl;
    btree.remove(2); //breaks things?
    cout << btree << endl;
    cout << "###########################" << endl;
    
    assert(btree.isValid());
}

//Random insertions to check the tree stays valid
void stress_insert(int itr) {
    // Stress-testing
    Btree btree;
    for (int i = 0; i < itr; ++i) {
        int num = rand() % itr*20;
        btree.insert(num);
        assert(btree.isValid());
    }
	
}

//Random deletes to check the tree stays valid
void stress_insert_delete(int itr) {
    // Stress-testing
    Btree btree;
    // insert a bunch of times - then delete it
    for (int i = 0; i < itr; ++i) {
        int num = rand() % itr*20;
        btree.insert(num);
        assert(btree.isValid());
    }
	cout << "done with stress insert " << endl;
    
    
    for (int i = 0; i < itr; ++i) {
        int num = rand() % itr*20;
	    cout << "i = " << i << endl;
	    cout << "num = " << num << endl;
        btree.remove(num);
        assert(btree.isValid());
	
	    cout << btree << endl;
    }
	cout << "done with stress delete" << endl;
}

void chris_testing()
{
	Btree t;
	/*
	//t.insert(6);
	////cout << "after inserting 6" << endl;
	////cout << t << endl << endl;

	//t.insert(5);
	////cout << "after inserting 5" << endl;
	////cout << t << endl << endl;

	//t.insert(3);
	////cout << "after inserting 3" << endl;
	////cout << t << endl << endl;

	//t.insert(7);
	////cout << "after inserting 7" << endl;
	////cout << t << endl << endl;

	//t.insert(8);
	////cout << "after inserting 8" << endl;
	////cout << t << endl << endl;

	//t.insert(2);
	////cout << "after inserting 2" << endl;
	////cout << t << endl << endl;

	////cout << "----------------------------------------------" << endl;

	//t.insert(4);
	////cout << "after inserting 4" << endl;
	////cout << t << endl << endl;

	//t.insert(1);
	////cout << "after inserting 1" << endl;
	////cout << t << endl << endl;

	//t.insert(11);
	////cout << "after inserting 11" << endl;
	////cout << t << endl << endl;

	//t.insert(0);
	////cout << "after inserting 0" << endl;
	////cout << t << endl << endl;

	////cout << "----------------------------------------------" << endl;

	//t.insert(13);
	//cout << "after inserting 13" << endl;
	//cout << t << endl << endl;
	//cout << "----------------------------------------------" << endl;
	*/

// 	t.insert(4);
// 	t.insert(7);
// 	t.insert(8);
// 	cout << "before remove: " << endl;
// 	cout << t << endl;
// 	t.remove_chris(4);

// 	vector<Data*> v = t.search_range(-3, 16);
// 	for (int i = 0; i < (int)v.size(); ++i)
// 	{
// 		cout << v[i]->value << ", ";
// 	}
// 	cout << endl;

}

int main() {
    std::string filename = std::string("expected_") + std::to_string(BTREE_FANOUT) + "_" + std::to_string(BTREE_LEAF_SIZE)
    + ".out";
    //freopen(filename.c_str(),"w",stdout); //Comment out if you want to write to a file. You should to set the
                                            //values in constants.h to create the corresponding output
    //---------------------------------3-2---------3-6
    small_test();					//passing!	//passing!
    splitTestFromSpec();			//passing!	//passing!
    testForRedistribution();		//passing!	//passing!
    large_test1();					//passing!	//passing!
    large_test2();					//passing!	//passing!
    stress_insert(500);				//passing!	//passing!
    stress_insert_delete(30);	//BREAKS AT INNER.123 on 3-2
	
	chris_testing(); //ignore me


    
    return 0;
}
