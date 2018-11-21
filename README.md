# SharedMemory

## Q&A
Q: What does the header do?  
A: This is a template for shared memory.

Q: How many platforms it support?  
A: It now support Ubuntu!

## How to use it?

```
#include <iostream>
#include "SharedMemory.h"

using namespace std;

class SharedMemoryData
{
    public:
        int version = 0;
        int value1 = 1;
        int value2 = 2;
        int value3 = 3;
};

int main()
{
    try{
        auto ptr = new SharedMemory<SharedMemoryData>("/home/jinming/Desktop/test", 2333, Permission::CREATE_RW);

        int a,b,c;
        SharedMemoryData * data = new SharedMemoryData();
        while(true)
        {
            cout << "input for a, b, c" << endl;
            cin >> a >> b >> c;
            data->value1 = a;
            data->value2 = b;
            data->value3 = c;
            data->version++;
            ptr->write(data);
        }
    }
    catch (int error)
    {
        cout << error << endl;
    }

    return 0;
}
```

```
#include <iostream>
#include "SharedMemory.h"

using namespace std;

class SharedMemoryData
{
    public:
        int version = 0;
        int value1 = 1;
        int value2 = 2;
        int value3 = 3;
};

int main()
{
    try{

        auto ptr = new SharedMemory<SharedMemoryData>("/home/jinming/Desktop/test", 2333, Permission::CREATE_RW);

        int version = 0;
        SharedMemoryData * data = new SharedMemoryData();
        while(true)
        {
            ptr->read(data);
            if(data->version != version)
            {
                cout << data->value1 << endl;
                cout << data->value2 << endl;
                cout << data->value3 << endl;
                version = data->version;
            }
        }
    }
    catch (int error)
    {
        cout << error << endl;
    }

    return 0;
}
```
