# persistent btree (incomplete) implementation
Run `make`, then run `./btree`. It'll open up something that will allow you to test it using user input.  
Entering `1` as input will allow you to see the page file creation process.  
From entering `2` after running, you'll see that each entry has a pointer to the next entry.  

What it does right now is write to a page file called page_file, and use that to access records and shit. You can see more in the code.

## Outputs:
Everything is built from these lines:
```
typedef int PageId;
typedef int SlotId;

// i.e. record = key,value; // pair <PageId,SlotId>
typedef std::pair <PageId,SlotId> RecordId; 
```

For reference, here is the main:
```
int main(int argc, char**argv) {
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
    std::cout << "Number of elements: " << num_lines << std::endl;
    std::cout << "Number of entries: " << num_entries << std::endl;

    std::cout << std::endl;
    
    std::cout << "Building tree from page file." << std::endl;
    std::string filename = PAGE_FILE_NAME;

    //BTreeIndex *bt = new BTreeIndex();

    return 0;
}
```

And here is part of the output from that, for a csv with 1000 entries:
```
Page file created.
Number of pages: 181
Header: Emp ID,First Name,Last Name,SSN,User Name,Password
Header [0]: Emp ID
Header [1]: First Name
Header [2]: Last Name
Header [3]: SSN
Header [4]: User Name
Header [5]: Password
Entry length: 6
Number of elements: 4996
Number of entries: 832
```
It seems like something is bugging out when it's writing the entries, but I don't know where.

Example output for an inserted string. Only the page it's on is updated:
```
---- c\{n_h4E\ ----
PageId for page 0: 438
SlotId for page 0: 453
PageId for page 1: 440
SlotId for page 1: 455
PageId for page 2: 441
SlotId for page 2: 454
PageId for page 3: 447
SlotId for page 3: 448
PageId for page 4: 442
SlotId for page 4: 453
PageId for page 5: 445
SlotId for page 5: 450
PageId for page 6: 438
SlotId for page 6: 461
PageId for page 7: 446
SlotId for page 7: 449
PageId for page 8: 435
SlotId for page 8: 460
PageId for page 9: 443
SlotId for page 9: 456
PageId for page 10: 440
SlotId for page 10: 456
PageId for page 11: 432
SlotId for page 11: 464
PageId for page 12: 439
SlotId for page 12: 457
PageId for page 13: 434
SlotId for page 13: 466
PageId for page 14: 443
SlotId for page 14: 449
PageId for page 15: 433
SlotId for page 15: 459
PageId for page 16: 432
SlotId for page 16: 464
PageId for page 17: 435
SlotId for page 17: 461
PageId for page 18: 440
SlotId for page 18: 456
PageId for page 19: 440
SlotId for page 19: 456
PageId for page 20: 431
SlotId for page 20: 465
PageId for page 21: 445
SlotId for page 21: 447
PageId for page 22: 445
SlotId for page 22: 447
PageId for page 23: 438
SlotId for page 23: 454
PageId for page 24: 445
SlotId for page 24: 447
PageId for page 25: 434
SlotId for page 25: 462
PageId for page 26: 444
SlotId for page 26: 448
PageId for page 27: 433
SlotId for page 27: 463
PageId for page 28: 449
SlotId for page 28: 451
PageId for page 29: 440
SlotId for page 29: 456
PageId for page 30: 436
SlotId for page 30: 460
PageId for page 31: 437
SlotId for page 31: 455
PageId for page 32: 443
SlotId for page 32: 449
PageId for page 33: 444
SlotId for page 33: 452
PageId for page 34: 432
SlotId for page 34: 464
PageId for page 35: 437
SlotId for page 35: 459
PageId for page 36: 432
SlotId for page 36: 464
PageId for page 37: 434
SlotId for page 37: 466
PageId for page 38: 438
SlotId for page 38: 462
PageId for page 39: 439
SlotId for page 39: 457
PageId for page 40: 439
SlotId for page 40: 457
PageId for page 41: 438
SlotId for page 41: 458
PageId for page 42: 439
SlotId for page 42: 457
PageId for page 43: 430
SlotId for page 43: 466
PageId for page 44: 436
SlotId for page 44: 460
PageId for page 45: 433
SlotId for page 45: 467
PageId for page 46: 443
SlotId for page 46: 461
PageId for page 47: 442
SlotId for page 47: 454
PageId for page 48: 439
SlotId for page 48: 457
PageId for page 49: 437
SlotId for page 49: 455
PageId for page 50: 430
SlotId for page 50: 466
PageId for page 51: 438
SlotId for page 51: 454
PageId for page 52: 438
SlotId for page 52: 462
PageId for page 53: 440
SlotId for page 53: 452
PageId for page 54: 441
SlotId for page 54: 451
PageId for page 55: 443
SlotId for page 55: 449
PageId for page 56: 435
SlotId for page 56: 465
PageId for page 57: 441
SlotId for page 57: 455
PageId for page 58: 442
SlotId for page 58: 458
PageId for page 59: 441
SlotId for page 59: 451
PageId for page 60: 438
SlotId for page 60: 462
PageId for page 61: 441
SlotId for page 61: 455
PageId for page 62: 441
SlotId for page 62: 455
PageId for page 63: 438
SlotId for page 63: 458
PageId for page 64: 440
SlotId for page 64: 456
PageId for page 65: 446
SlotId for page 65: 450
PageId for page 66: 444
SlotId for page 66: 456
PageId for page 67: 441
SlotId for page 67: 451
PageId for page 68: 442
SlotId for page 68: 458
PageId for page 69: 440
SlotId for page 69: 460
PageId for page 70: 438
SlotId for page 70: 454
PageId for page 71: 439
SlotId for page 71: 457
PageId for page 72: 443
SlotId for page 72: 453
PageId for page 73: 443
SlotId for page 73: 457
PageId for page 74: 437
SlotId for page 74: 459
PageId for page 75: 438
SlotId for page 75: 458
PageId for page 76: 431
SlotId for page 76: 461
PageId for page 77: 445
SlotId for page 77: 447
PageId for page 78: 438
SlotId for page 78: 466
PageId for page 79: 434
SlotId for page 79: 458
PageId for page 80: 445
SlotId for page 80: 451
PageId for page 81: 438
SlotId for page 81: 458
PageId for page 82: 439
SlotId for page 82: 461
PageId for page 83: 447
SlotId for page 83: 449
PageId for page 84: 443
SlotId for page 84: 449
PageId for page 85: 446
SlotId for page 85: 454
PageId for page 86: 432
SlotId for page 86: 468
PageId for page 87: 433
SlotId for page 87: 467
PageId for page 88: 448
SlotId for page 88: 452
PageId for page 89: 443
SlotId for page 89: 453
PageId for page 90: 446
SlotId for page 90: 454
PageId for page 91: 430
SlotId for page 91: 466
PageId for page 92: 444
SlotId for page 92: 448
PageId for page 93: 442
SlotId for page 93: 454
PageId for page 94: 435
SlotId for page 94: 461
PageId for page 95: 441
SlotId for page 95: 455
PageId for page 96: 446
SlotId for page 96: 454
PageId for page 97: 433
SlotId for page 97: 463
PageId for page 98: 438
SlotId for page 98: 450
PageId for page 99: 444
SlotId for page 99: 452
PageId for page 100: 440
SlotId for page 100: 457
PageId for page 101: 441
SlotId for page 101: 460
PageId for page 102: 442
SlotId for page 102: 459
PageId for page 103: 439
SlotId for page 103: 454
PageId for page 104: 445
SlotId for page 104: 452
PageId for page 105: 432
SlotId for page 105: 465
PageId for page 106: 441
SlotId for page 106: 452
PageId for page 107: 438
SlotId for page 107: 463
PageId for page 108: 441
SlotId for page 108: 460
PageId for page 109: 443
SlotId for page 109: 450
PageId for page 110: 445
SlotId for page 110: 448
PageId for page 111: 435
SlotId for page 111: 466
PageId for page 112: 443
SlotId for page 112: 450
PageId for page 113: 440
SlotId for page 113: 453
PageId for page 114: 442
SlotId for page 114: 455
PageId for page 115: 442
SlotId for page 115: 451
PageId for page 116: 439
SlotId for page 116: 458
PageId for page 117: 440
SlotId for page 117: 461
PageId for page 118: 439
SlotId for page 118: 458
PageId for page 119: 447
SlotId for page 119: 450
PageId for page 120: 445
SlotId for page 120: 456
PageId for page 121: 439
SlotId for page 121: 458
PageId for page 122: 437
SlotId for page 122: 460
PageId for page 123: 444
SlotId for page 123: 457
PageId for page 124: 444
SlotId for page 124: 453
PageId for page 125: 441
SlotId for page 125: 452
PageId for page 126: 442
SlotId for page 126: 455
PageId for page 127: 438
SlotId for page 127: 459
PageId for page 128: 444
SlotId for page 128: 453
PageId for page 129: 440
SlotId for page 129: 457
PageId for page 130: 441
SlotId for page 130: 452
PageId for page 131: 432
SlotId for page 131: 469
PageId for page 132: 436
SlotId for page 132: 461
PageId for page 133: 438
SlotId for page 133: 463
PageId for page 134: 444
SlotId for page 134: 457
PageId for page 135: 439
SlotId for page 135: 454
PageId for page 136: 443
SlotId for page 136: 454
PageId for page 137: 436
SlotId for page 137: 465
PageId for page 138: 433
SlotId for page 138: 468
PageId for page 139: 438
SlotId for page 139: 459
PageId for page 140: 438
SlotId for page 140: 459
PageId for page 141: 444
SlotId for page 141: 449
PageId for page 142: 436
SlotId for page 142: 465
PageId for page 143: 447
SlotId for page 143: 450
PageId for page 144: 437
SlotId for page 144: 464
PageId for page 145: 447
SlotId for page 145: 450
PageId for page 146: 439
SlotId for page 146: 458
PageId for page 147: 436
SlotId for page 147: 461
PageId for page 148: 436
SlotId for page 148: 465
PageId for page 149: 438
SlotId for page 149: 463
PageId for page 150: 436
SlotId for page 150: 461
PageId for page 151: 439
SlotId for page 151: 462
PageId for page 152: 439
SlotId for page 152: 462
PageId for page 153: 440
SlotId for page 153: 453
PageId for page 154: 443
SlotId for page 154: 454
PageId for page 155: 447
SlotId for page 155: 450
PageId for page 156: 440
SlotId for page 156: 461
PageId for page 157: 448
SlotId for page 157: 449
PageId for page 158: 448
SlotId for page 158: 449
PageId for page 159: 446
SlotId for page 159: 447
PageId for page 160: 445
SlotId for page 160: 452
PageId for page 161: 434
SlotId for page 161: 463
PageId for page 162: 439
SlotId for page 162: 454
PageId for page 163: 444
SlotId for page 163: 449
PageId for page 164: 445
SlotId for page 164: 448
PageId for page 165: 446
SlotId for page 165: 455
PageId for page 166: 434
SlotId for page 166: 463
PageId for page 167: 445
SlotId for page 167: 452
PageId for page 168: 448
SlotId for page 168: 449
PageId for page 169: 445
SlotId for page 169: 452
PageId for page 170: 436
SlotId for page 170: 461
PageId for page 171: 431
SlotId for page 171: 470
PageId for page 172: 438
SlotId for page 172: 463
PageId for page 173: 433
SlotId for page 173: 464
PageId for page 174: 445
SlotId for page 174: 456
PageId for page 175: 433
SlotId for page 175: 468
PageId for page 176: 445
SlotId for page 176: 456
PageId for page 177: 429
SlotId for page 177: 460
PageId for page 178: 438
SlotId for page 178: 467
PageId for page 179: 441
SlotId for page 179: 460
PageId for page 180: 436
SlotId for page 180: 461
PageId for page 181: 424
SlotId for page 181: 477
```


