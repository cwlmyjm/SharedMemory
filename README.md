# SharedMemory

## Q&A
Q: What does the header do?  
A: This is a template for shared memory.

Q: How many platforms it support?  
A: It only support Win now.

## How to use it?

```
#include <iostream>
#include "SharedMemory.h"

using namespace std;

int main()
{
	try{
		auto sm = new SharedMemory<int>("SharedMemory", CREATE_RW);
		int a = 100;
		while (true)
		{
			sm->write(&a);
			cout << a << endl;
			a++;
		}
	}
	catch (int error)
	{
		cout << error << endl;
	}
	system("pause");
    return 0;
}
```

```
#include <iostream>
#include "SharedMemory.h"

using namespace std;

int main()
{
	try
	{
		auto sm = new SharedMemory<int>("SharedMemory", OPEN_RW);
		int a = 100;
		while (true)
		{
			sm->read(&a);
			cout << a << endl;
		}
	}
	catch (int error)
	{
		cout << error << endl;
	}
	system("pause");
    return 0;
}
```
