#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <sstream>
#include <map>
#include <utility>
#include <set>
#include <cmath>
#include <iomanip>
#include <cstdlib>
#include <algorithm>
#include <memory>
using namespace std;

struct fpNode{
    string name;
    int val = 0;
    vector<shared_ptr<fpNode>> child{};
    shared_ptr<fpNode> parent = NULL;
    shared_ptr<fpNode> next = NULL;
    fpNode(string s): name(s), next(NULL) {};
    fpNode(string s, int v, shared_ptr<fpNode> n): name(s), val(v), parent(n), next(NULL) {};
};
struct link {
    //string name;
    int freq = 0;
    shared_ptr<fpNode> head = NULL;
    shared_ptr<fpNode> tail = NULL;
};

bool build_table(string, int&, map<string, link>&, vector<pair<string, int>>&, float&);
void constructFP(string, shared_ptr<fpNode>, map<string, link>&, int);
shared_ptr<fpNode> conditional_tree(map<string, link>&, vector<pair<vector<string>, int>>&, int);
void create_pattern(shared_ptr<fpNode> , map<string, link>&, int, string&, vector<string>, map<vector<string>, int>&);
map<vector<string>, int> mine(shared_ptr<fpNode> , map<string, link>&, vector<pair<string, int>>&, int, int);
void write_file(map<vector<string>, int>&, string, int); 

bool comp_int_string (const pair<string, int> a, const pair<string, int> b) {
	if(a.second == b.second) {
        return a.first < b.first;
    }
    return a.second < b.second;
}
bool comp_greater_int (const pair<string, int> a, const pair<string, int> b) {
	if(a.second == b.second) {
        return a.first < b.first;
    }
    return a.second > b.second;
}
std::string round_4digit(float num) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(4) << num;
    return ss.str();
}
shared_ptr<fpNode> find_child(vector<shared_ptr<fpNode>>& child, string target) {
    for(auto it = child.begin(); it != child.end(); it++) {
        if((*it)->name == target) {
            return (*it);
        } 
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    
    float min_sup_pct = atof(argv[1]); 
    string input_file(argv[2]), output_file(argv[3]); // using constructor
    int total_transaction = 0; // get value in build_table
    map<string, link> header_table; // item / frequency / head
    vector<pair<string, int>> frequency; // get value in build_table
    if(!build_table(input_file, total_transaction, header_table, frequency, min_sup_pct)) {
        return 1;
    }
    int min_sup = (int) ceil(total_transaction * min_sup_pct);
    
    sort(frequency.begin(), frequency.end(), comp_int_string); // sort by frequency and then sort by string

    //fpNode* root = new fpNode("root");
    std::shared_ptr<fpNode> root(new fpNode("root"));

    constructFP(input_file, root, header_table, min_sup);
    map<vector<string>, int>ans = mine(root, header_table, frequency, min_sup, total_transaction);
    write_file(ans, output_file, total_transaction);
    return 0;
}

bool build_table(string input_file, int& transaction, map<string, link>& header_table, vector<pair<string, int>>& freqency_vec, float& min_support_pct) {
   
    std::ifstream ifs(input_file, std::ios::in); // scan Database
    if (!ifs.is_open()) {
        cout << "Failed to open file.\n";
        return false; // EXIT_FAILURE
    }
    string line;
    while(std::getline(ifs, line)) { // get "2,1,3" in line
        stringstream ss(line); // convert string into stringstream
        string tmp;
        transaction++;
        while(getline(ss, tmp, ',')) { // split to 2 1 3 in ss
            header_table[tmp].freq++;
        }        
    }
    int min_sup = (int) ceil(transaction * min_support_pct);
    for(auto x: header_table) {
        if(x.second.freq >= min_sup) {
            freqency_vec.push_back(make_pair(x.first, x.second.freq));
        }       
    }
    return true;
}
void constructFP(string input_file, shared_ptr<fpNode> root, map<string, link>& header_table, int min_sup) {
    std::ifstream ifs(input_file, std::ios::in); // Scan database
    if (!ifs.is_open()) {
        cout << "Failed to open file.\n";
        return; // EXIT_FAILURE
    }
    string line;
    while(std::getline(ifs, line)) { // get a transaction
        stringstream ss(line); // convert string into stringstream
        string tmp_name;
        shared_ptr<fpNode> loc = root;
        vector<pair<string, int>> sorted_t;
        while(getline(ss, tmp_name, ',')) { // take every transaction into tree
            if(header_table[tmp_name].freq >= min_sup) {
                sorted_t.push_back(make_pair(tmp_name, header_table[tmp_name].freq));
            }           
        }
        sort(sorted_t.begin(), sorted_t.end(), comp_greater_int); // sort by descending frequency and ascending string

        for(auto item: sorted_t) {
            tmp_name = item.first;
            link* h_item = &header_table[tmp_name];         
            if(h_item->freq >= min_sup) { // scan db again so we need to filter
                shared_ptr<fpNode> node  = find_child(loc->child, tmp_name);
                if(node != NULL) { // the node has child and we find it                     
                    node->val++;
                    loc = node; // go down 1 level                                   
                } else { // no child
                    shared_ptr<fpNode> tmp(new fpNode(tmp_name, 1, loc)); // child vector is empty
                    loc->child.push_back(tmp);
                    loc = tmp; // go down 1 level
                }
                if(h_item->head == NULL) { // update head
                    h_item->head = loc;
                }
                if(h_item->tail == NULL) { // update tail and horizontal link
                    h_item->tail = loc;
                } else if(loc->val == 1) { // make loc the new tail
                    h_item->tail->next = loc;                   
                    h_item->tail = loc;
                }
            } 
        }
    }
}

shared_ptr<fpNode> conditional_tree(map<string, link>& element_table, vector<pair<vector<string>, int>>& pattern_base, int min_support){
    if(pattern_base.empty()) {
        return NULL;
    }
    shared_ptr<fpNode> root(new fpNode("root"));  
    for(int i = 0; i < pattern_base.size(); i++) {
        vector<string> a_pattern = pattern_base[i].first; // transaction
        int value_pattern = pattern_base[i].second; // transaction value
        shared_ptr<fpNode> loc = root;
        for(int j = a_pattern.size()-1; j >= 0; j--) {
            link* h_item = &element_table[a_pattern[j]];
            if(h_item->freq >= min_support) {
                shared_ptr<fpNode> node = find_child(loc->child, a_pattern[j]);
                if(node != NULL) { // is a child
                    node->val += value_pattern;
                    loc = node;
                } else { // not a child then create new node
                    shared_ptr<fpNode> tmp(new fpNode(a_pattern[j], value_pattern, loc));
                    loc->child.push_back(tmp);
                    loc = tmp;
                }
                if(h_item->head == NULL) {
                    h_item->head = loc;
                }
                if(h_item->tail == NULL) { // update tail and horizontal link
                    h_item->tail = loc;
                } else if((loc->next == NULL) && (loc != h_item->tail)) { // make loc the tail
                    h_item->tail->next = loc;                   
                    h_item->tail = loc;
                }
            }   
        }
    }
    return root;
}

void create_pattern(shared_ptr<fpNode> root, map<string, link>& header_table, int min_support, string& current_suffix, vector<string> ready_suffix, map<vector<string>, int>& pattern_support){
    ready_suffix.push_back(current_suffix);
    int suffix_val = header_table[current_suffix].freq;
    
    map<string, link> element_count_table; // header table for conditional FP tree and recursion
    vector<pair<vector<string>, int>> pattern_base; // path from leaf which contains conditional  pattern base
    shared_ptr<fpNode> leaf = header_table[current_suffix].head;
    for(; leaf!= NULL; leaf = leaf->next) {
        int prev_val = leaf->val;
        // go through each node of one path
        vector<string> tmp_path;
        for(shared_ptr<fpNode> node = leaf->parent; node->name != "root"; node = node->parent) {
            element_count_table[node->name].freq += prev_val; // count items
            tmp_path.push_back(node->name);
        }      
        if(!tmp_path.empty()) {
            pattern_base.push_back(make_pair(tmp_path, prev_val)); // make pattern base (transactions)
        }
    }
    if(pattern_base.empty()) {
        pattern_support[ready_suffix] = suffix_val; // or change this at beginning of the function?
        return;
    }
    // make conditional FP tree here 
    shared_ptr<fpNode> fp_tree = conditional_tree(element_count_table, pattern_base, min_support); // conditional FP tree root
    if(fp_tree == NULL) {
        return;
    }
    vector<pair<string, int>> frequency; // go through frequency as suffix for recursion
    for(auto x: element_count_table) {
        if(x.second.freq >= min_support) { // to make the vector with supported items for further recursion
            frequency.push_back(make_pair(x.first, x.second.freq));
        }      
    }
    sort(frequency.begin(), frequency.end(), comp_int_string); // start from low frequency item
    // use all supported items to Recursion
    for(auto product: frequency) {
        create_pattern(fp_tree, element_count_table, min_support, product.first, ready_suffix, pattern_support);
        
    } 
    pattern_support[ready_suffix] = suffix_val; // add the original pattern
}

map<vector<string>, int> mine(shared_ptr<fpNode> root, map<string, link>& header_table, vector<pair<string, int>>& freq_vec, 
                                int min_support, int total_transaction) {                                   
    map<vector<string>, int> pattern_support;
    for(auto suf: freq_vec) {
        string suffix = suf.first; // suffix string
        create_pattern(root, header_table, min_support, suffix ,{}, pattern_support);
    }
    return pattern_support;
}

void write_file(map<vector<string>, int>& ans, string output_file, int total_transaction) {
    ofstream myfile;
    myfile.open(output_file);
    //cout << "-------write file--" << endl;
    int count = 0; // for checking answer
    for(auto a: ans) {
        int n = a.first.size();
        for(int i = 0; i < n-1; i++) {
            myfile << a.first[i] << ",";
        }
        myfile << a.first[n-1] << ":";
        myfile << round_4digit(a.second * 1.0 / total_transaction) << endl;
        count++;
    }
    //cout << "total: " << count << endl;
    myfile.close();
}
