#include <iostream>
#include <string.h>
using namespace std;

bool check(int * copy_frames, int frames, int entries)
{
  for (int i = 0; i < frames; i++)
  {
    if(entries == copy_frames[i])
      return true;
  }
  return false;
}

void extend(int* tracker, int frames)
{
  for (int i = 0; i < frames; i++)
    tracker[i]++;
}

int ready(int* test, int size)
{
  int max = 0;
  int index = 0;
  for (int i = 0; i < size; i++)
  {
    if (test[i] > max)
{
      max = test[i];
      index = i;
    }
  }
  return index;
}

void CLOCK(int* entries, int length, int frames){
  int kill_pointer = 0;
  int reference_bit[frames];
  int copy_frames[frames];
  int fault_count = 0;

  for (int i = 0; i < frames; i++)
  {
    copy_frames[i] = entries[i];
    fault_count++;
    reference_bit[i] = 1;
  }

  for (int i = fault_count; i < length; i++)
  {
    if(!check(copy_frames, frames, entries[i]))
	{
      while(reference_bit[kill_pointer] != 0)
	  {
		reference_bit[kill_pointer] = 0;
		kill_pointer++;
		if(kill_pointer >= frames)
		  kill_pointer = 0;
      }
      copy_frames[kill_pointer] = entries[i];
      fault_count++;
      reference_bit[kill_pointer] = 1;
    }
    int reset = 0;
    while(copy_frames[reset] != entries[i])
      reset++;

    reference_bit[reset] = 1;
  }

  cout << "CLOCK:\n  - frames: ";
  for(int i = 0; i < frames; i++){
    cout << copy_frames[i] << " ";
  }
  cout << "\n  - page faults: " << fault_count << endl;
}
void LRU(int* entries, int length, int frames)
{
  int kill_pointer;
  int tracker[frames];
  int copy_frames[frames];
  int fault_count = 0;
  
  for (int i = 0; i < frames; i++)
  {
    tracker[i] = 0;
  }

  for (int i = 0; i < frames; i++)
  {
    copy_frames[i] = entries[i];
    fault_count++;
	
    for (int j = i; j >= 0; j--)
	{
      tracker[j]++;
    }
  }

  for (int i = fault_count; i < length; i++)
  {
    if(!check(copy_frames, frames, entries[i]))
	{
      kill_pointer = ready(tracker, frames);
      copy_frames[kill_pointer] = entries[i];
      tracker[kill_pointer] = 0;
      fault_count++;
    }
    else
	{
      int j = 0;
      while(copy_frames[j] != entries[i])
		  
	j++;
      tracker[j] = 0;
    }
    extend(tracker, frames);
  }

  cout << "LRU:\n  - frames: ";
  for(int i = 0; i < frames; i++)
  {
    cout << copy_frames[i] << " ";
  }
  cout << "\n  - page faults: " << fault_count << endl;
}


void optimal(int* entries, int length, int frames)
{
  
  int max = 0;
  int kill_pointer ;
  int copy_frames [frames];
  int fault_count = 0;
 
//initial copies are counted as page faults 

int i = 0;
int k = 0; 

  while(k < frames)
  {	
	int hit = 0;
	// checking if there is a copy already in here 
	for (int j = 0; j < k; j++) 
	{
		if(copy_frames [j] == entries[i])
		{
			hit = 1;
			cout <<"out here"<<endl;
		}
	}
	
	if(hit == 0)
	{	
		copy_frames [k] = entries[i];
		fault_count++;
		k++;
	}
	i++;
	
  }
// when the frames become full, 
  for (int i = fault_count; i < length; i++)
  {
    if(!check(copy_frames , frames, entries[i]))
	{
      for (int j = 0; j < frames; j++)
	  {
		int pointer = i + 1;
		
		while(copy_frames [j] != entries[pointer] && pointer < length)
		{
		  pointer++;
		}
		
		if( (pointer - i) > max )
		{
		  max = pointer - i;
		  kill_pointer  = j;
		}
      }
      copy_frames [kill_pointer ] = entries[i];
      fault_count++;
      max = 0;
    }
  }

  cout << "Optimal:\n  - frames: ";
  for(int i = 0; i < frames; i++){
    cout << copy_frames [i] << " ";
  }
  cout << "\n  - page faults: " << fault_count << endl;
}

int main (int argv, char **argc)
{
	if (argv != 2)
	{
		cout << "Error: Please check your inputs";
		exit(-1);
    }
	
	
	int length = 0;
	int entries[5000];
	int frames = atoi(argc[1]);
	char* buffer;
	
	while (cin >> buffer)
	{
		entries[length] = atoi(buffer);
		length++;
    }
	
	optimal(entries, length, frames);
	LRU(entries, length, frames);
	CLOCK(entries, length, frames);
	
	

}