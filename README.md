# SharedMemory

## Q&A
Q: What does the header do?  
A: This is a template for shared memory.

Q: How many platforms it support?  
A: It only support Windows now.

Q: Does it has mutex in reading or writing?  
A: No, you can implement mutex by yourself.

Q: How to create a SharedMemory with specific length, like 10 bytes?  
A: Define a struct contian 10 bytes or use char\[10\] as T.

```
Example one
    struct TenBytes{
        char[10] data;
    }
    SharedMemory<TenBytes> sm("SharedMemory", CREATE_RW);
Example two
    SharedMemory<char[10]> sm("SharedMemory", CREATE_RW);
```

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
