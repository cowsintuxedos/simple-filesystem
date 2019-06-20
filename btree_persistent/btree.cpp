#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>
#include <tuple>
#include <sstream>
#include <fstream>
#include <iterator>
#include <cstdlib>

// andrew lai 9519687

// read input file
// store file on disk (bulk load)

typedef int PageId;
typedef int SlotId;

// i.e. record = key,value; // pair <PageId,SlotId>
typedef std::pair <PageId,SlotId> RecordId; 

// page offsets
std::vector<RecordId> records; // set by PageFile, used by RecordFile

// at most h+1 pages stored in memory, where h = height of tree
static const int PAGE_SIZE = 1024;

// first read page into buffer pool in main memory
static const int BUFFER_SIZE = 10;

static const std::string PAGE_FILE_NAME = "page_file";

// csv header information
std::vector<std::string> header;
std::string header_string;
int entry_length = 0; // defines how many words in an entry based on header size

int height = 0, num_lines = 0, num_pages = 0, num_entries = 0;

// provide page-level access to unix filesystem
class PageFile {
    // helpers and variables
    std::fstream input;
    std::ofstream output;

    // lines = # of lines in file, pages = # of pages. wow
    int lines, pages;

    // 'r' if read, 'w' if write
    char cur_mode;

    PageId cur_page = -1; // num_pages = cur_page + 1;
    int cur_buffer = 0, header_offset = 0, bucket_offset = 0;

    // create new empty page in with PageId header_offset, SlotId bucket_offset
    void new_page() {
        cur_page++;

        //fill page with empty space so we can seek anywhere
        std::string spaces(PAGE_SIZE, ' ');
        output.seekp(PAGE_SIZE * cur_page);
        output << spaces;

        //write in current page
        output.seekp(PAGE_SIZE * cur_page);
        output << cur_page << " "<<std::endl;

        //set header and index offsets // pid and sid
	    header_offset = ((int)output.tellp()-1) % PAGE_SIZE;

        output.seekp(PAGE_SIZE * (cur_page+1));
	    bucket_offset = PAGE_SIZE;
	    // std::cout << "bucket_offset at page creation: " << bucket_offset << std::endl;
	    output.flush(); // TODO: clean this up for BUFFER_SIZE

        // std::cout << "header_offset = " << header_offset << ", bucket_offset = " << bucket_offset << std::endl;
	    records.push_back(RecordId(header_offset, bucket_offset)); // for use by RecordFile class
    }

    // add new word to index and return its pid
    // (1) check if current page has enough space; create new page if needed
    // (2) write the word and update the records
    PageId new_word(std::string word) {

        // cur_record: pair<PageId, SlotId>
        RecordId cur_record = records.at(cur_page);
        // std::cout << "cur_record" << std::endl << std::get<1>(cur_record) << std::endl << std::get<0>(cur_record) << std::endl;

        // bucket pointer
        std::string next_bucket = "0000000";

        // bucket = word + pointer to next bucket + newline
        int bucket_size = word.length() + next_bucket.length() + sizeof(' ') + sizeof('\n');
        int new_bucket_offset = std::get<1>(cur_record) - bucket_size; // get new bucket offset
        int header_size = word.length() + sizeof('\xFB') + std::to_string(new_bucket_offset).length() + sizeof(' ');
        // std::cout << "bucket_size: " << bucket_size << ", for word: " << word << std::endl;

        // check if bucket and header entry will fit into current page; create new page & point to it if needed
        if (bucket_size + header_size > std::get<1>(cur_record) - std::get<0>(cur_record) - 1) {
            std::cout << "creating new page in new_word for cur_page: " << cur_page << std::endl;
            //std::cout << bucket_size << std::endl;
            //std::cout << header_size << std::endl;
            //std::cout << std::get<1>(cur_record) << std::endl;
            //std::cout << std::get<0>(cur_record) << std::endl;

            new_page();
            cur_record = records.at(cur_page);
            new_bucket_offset = std::get<1>(cur_record) - bucket_size;
        }

        // write header entry
        output.seekp(cur_page * PAGE_SIZE + std::get<0>(cur_record));
        output << word << "\xFB" << new_bucket_offset << " " << std::endl;
        header_offset = ((int)output.tellp()-1) % PAGE_SIZE;

        // write bucket entry
        output.seekp(cur_page * PAGE_SIZE + new_bucket_offset);
        output << word << ' ' << next_bucket << std::endl; // see bucket_size, check equal
        output.flush(); // TODO: clean this up for BUFFER_SIZE

        // update records
        records[cur_page] = RecordId(header_offset, new_bucket_offset);
        return cur_page;
    }

    void print_offsets() {
        for(int i = 0; i < records.size(); i++){
		    std::cout << "PageId for page " << i << ": " << std::get<0>(records[i]) << std::endl;
		    std::cout << "SlotId for page " << i << ": " << std::get<1>(records[i]) << std::endl;
	    }
    }

// accessors
public:
    PageFile() {
        std::cout << "PageFile initialized successfully." << std::endl;
    }

    // open file in read or write mode
    // if open non-existing file in write mode, 
    // automatically create file with given name
    void open(const std::string &filename, char mode) {

        // if read mode
        if (mode == 'r') {
            input.open(filename);
            output.open("page_file");
            
            new_page(); // create new page to write to

            if (input.is_open()) {
                cur_mode = mode;
                std::string line;

                // get header line
                std::getline(input, line);
                std::cout << line << std::endl;
                header_string = line;
                std::stringstream ss(line);
                std::string elem;
                while (std::getline(ss, elem, ',')) header.push_back(elem);

                // read file line by line
                while(std::getline(input, line)){
                    //get a vector of all the words in the line
  		            std::istringstream iss(line);
  		            std::vector<std::string> cur_line((std::istream_iterator<std::string>(iss)), std::istream_iterator<std::string>());
                    entry_length = cur_line.size();
                    // TODO: later set to do in sets of BUFFER_SIZE
                    for (size_t i = 0; i < cur_line.size(); i++) {
                        std::string cur_word = cur_line[i];
                        std::cout << "---- " << cur_word << " ----" << std::endl;
                        print_offsets();

                        new_word(cur_word);

                        std::cout << std::endl;
                    }

                    lines++;
                }

            } else { std::cout << "Unable to read file." << std::endl; }
       
        // else if write mode
        } else if (mode == 'w') {
            output.open(filename);
            cur_mode = mode;
        } else { std::cout << "Incorrect mode." << std::endl; }

        // set global variables accordingly
        num_lines = lines;
        num_pages = cur_page;
        entry_length = header.size(); // set size of entry for accessing data

    }

    // close file
    void close() {
        if (cur_mode == 'r') {
            input.close();
            output.close();
        }
        else if (cur_mode == 'w')
            output.close();
        else
            std::cout << "No file open." << std::endl;
    }

    // read a page in the file into main memory
    // some PageId of page + pointer to buffer where content loaded
    void read(PageId pid, void *buffer) {

    }

    // return ID of page immediately after last page in file
    // use to scan the file
    PageId endPid() {
        return pages;
    }

    // write content in main memory to a page in the file
    void write(PageId pid, const void *buffer) {

    }
    // PageId: 0  1  2  3  ...
    //         [ ][ ][ ][ ][ ][ ][ ][ ][ ][ ]
};

// PageId:     0          1          2  3  ... 11
// SlotId:   0 [key,value][key,value][ ][ ][ ][key,value]
//           1 [key,value][key,value][ ][ ][ ][key,value]
//           . [...      ][key,value][ ][ ][ ][key,value]
//             [key,value][key,value][ ][ ][ ][key,value]

// provide record-level access to a file
// each page split into slots, each record in one slot
// location = (PageId, SlotId) pair
class RecordFile {

    // open file
    // if writing non-existent file,
    // create file w/ given name
    void open() {

    }

    // close file
    void close() {

    }

    // read record at RecordId from file
    std::string read(RecordId rid) {

    }

    // insert new record at end of file
    void append(std::string record) {

    }

    // return RecordId of record immediately after last record in file
    RecordId endRid() {

    }
};

// every node in B+ tree = page in PageFile
// Pageid stored in non-leaf node = pointer to child node
// last PageId in leaf node of B+ tree = pointer to next sibling node
// int PageId = pointer to page in PageFile (node in B+ tree)
// int RecordId = pointer to record in RecordFile

// execute user commands
class Main {
    // RecordId = pair <PageId,SlotId>
    // i.e. record id = key,value;

    // insert record into the tree
    void insert(std::string record) {
        
    }

    // search for record & return the RecordId
    RecordId lookUp(std::string record) {

    }
};

class Node {
    int num_keys;
    std::vector<int> keys; // keys

    PageId next_node;
    bool is_leaf;
};

class BTreeIndex {
    public:
    BTreeIndex() {}

    // insert node
    void insert(Node* node) {
        
    }

    // retrieve node
    Node* retrieve() {

    }

    // build the tree
    void create_tree(){
        std::cout << "Creating tree. " << std::endl; 
        std::ifstream input;
        input.open(PAGE_FILE_NAME);
        if (input.is_open()) {
            std::cout << "File opened." << std::endl;
            int file_size = input.tellg();
            input.seekg(0, std::ios::end);
            file_size = (int)input.tellg() - file_size;
            int num_pages = file_size/PAGE_SIZE;
            std::cout << "number of pages in file: " << num_pages << std::endl;
            input.seekg(0);
            std::string line;
            for (int i = 0; i < num_pages; i++) {
                input.seekg(PAGE_SIZE*i);
                getline(input, line);
                // std::cout << tempLine << std::endl;
                std::istringstream stream_header(line);
                std::vector<std::string> header_entries((std::istream_iterator<std::string>(stream_header)), std::istream_iterator<std::string>());
                for (int j = 1; j < header_entries.size(); j++) {
                    std::vector<std::string> tokens;
                    std::string token;
                    std::istringstream token_stream(header_entries[j]);
                    while (std::getline(token_stream, token, '\xFB')) {
                        std::cout << "token: " << token << std::endl;
                        tokens.push_back(token);
                    }
                    input.seekg(PAGE_SIZE*i + stoi(tokens[1]));
                    std::string bucket_line;
                    getline(input, bucket_line);
                    std::cout << "page_file entry: " << bucket_line << std::endl; 
                    std::istringstream bucket_stream(bucket_line);
                    std::vector<std::string> bucket_entries((std::istream_iterator<std::string>(bucket_stream)), std::istream_iterator<std::string>());
                    std::string temp = bucket_entries[0]; // TODO
                    std::cout << "check: " << temp << std::endl; 
                    //for (int k = 2; k < bucket_entries.size(); k++) {
                    //  std::get<1>(tempTuple).push_back(bucket_entries[k]);
                    //}
                    //insert(temp);
                }
            }
            // Print the tree out after creation
            //traverse(root); // TODO
            //return root;
        } else {
            std::cerr << "Unable to open file" << std::endl;
            exit(0);
        }
    }
};

// cast as LeafNode to keep track of leaf state of node
class LeafNode : public Node {
    // read content of node from pid in pf
    void read(PageId pid, const PageFile& pf) {

    }

    // write content of node to pid in pf
    void write(PageId pid, PageFile& pf) {

    }

    // insert (key, rid) pair to the node
    void insert(int key, const RecordId& rid) {

    }

    // return pid of next sibling node
    int getNextNodePtr() {

    }

    // set pid of next sibling node
    void setNextNodePtr(int pid) {

    }
};

class NonLeafNode : public Node {
    // given searchkey, find child-node pointer to follow
    // & output it in pid
    int locateChildPtr(int searchKey, PageId& pid) {

    }

    // initialize root node with (pid1, key, pid2)
    void initializeRoot(PageId pid1, int key, PageId pid2) {

    }
};

// save height of tree, keep track of how many levels traversed
// add special flag to each node to indicate if leaf

// when inserting new entry into leaf-node, node may overflow
// req. new (key, pointer) pair inserted into parent

// keep track of sequence of nodes visited as traversing
// pay attention to what is stored in main memory vs stored in disk

int main(int argc, char**argv) {
    while (true) {
        std::cout << std::endl << "Choose an action:" << std::endl;
        std::cout << "  1: generates the pagefile" << std::endl;
        std::cout << "  2: builds the tree from a generated pagefile" << std::endl;
        //std::cout << "  insert:" << std::endl;
        std::cout << "  3: quits" << std::endl;
        std::cout << std::endl << "Action: " << std::endl;
        int input;
        std::cin >> input;

        if (input == 1) {
            std::cout << "Loading file." << std::endl;
            PageFile *pf = new PageFile();

            //std::string testfile = "SampleRecordssorted.csv";
            std::string testfile = "test.csv";
            //std::string testfile = "short.csv";
            pf->open(testfile, 'r');
            pf->close();

            std::cout << "Page file created." << std::endl;
            std::cout << "Number of pages: " << num_pages << std::endl;
            std::cout << "Header: " << header_string << std::endl;
            for (int i = 0; i < header.size(); i++) {
                std::cout << "Header [" << i << "]: " << header[i] << std::endl;
            }
            std::cout << "Entry length: " << entry_length << std::endl;
            std::cout << "Number of lines: " << num_lines << std::endl;

            std::cout << std::endl;
        }
        else if (input == 2) {
            std::cout << "Building tree from page file." << std::endl;
            std::string filename = PAGE_FILE_NAME;

            BTreeIndex *bt = new BTreeIndex();
            bt->create_tree();
        }
        else {
            std::cout << "Quitting." << std::endl;
            exit(0);
        }
    }


    return 0;
}