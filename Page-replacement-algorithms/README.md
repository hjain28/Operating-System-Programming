# Page-replacement-algorithms-Optimal-LRU-and-Clock.

Paging is the most common implementation of virtual memory. 

Page fault - is an exception raised when a process accesses a memory page that is not currently mapped by MMU, eg. entry in page table marked invalid

When operating system runs out of memory it must replace the pages in an effective manner taking multiple factors into consideration. 

Least Recently Used Algorithm (LRU) -> uses the past knowledge to predict the future. replaces the pages based on the frequency of use. 
If pages don't get used as much compared to others, it will be replaced. 

CLOCK Algorithm  -> Make use of the reference bit in page table entry, which is automatically set by hardware any time page is accessed. 
frames are organized as a circular buffer. If a page has reference bit = 0, replace it otherwise set reference bit to 0 and advance pointer to the next page
