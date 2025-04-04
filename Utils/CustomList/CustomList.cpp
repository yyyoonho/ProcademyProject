#include <iostream>
#include "CustomList.h"

using namespace std;

int main()
{
    MyList<int> myList;

    for (int i = 0; i < 10; i++)
    {
        myList.push_front(i);
    }

    MyList<int>::iterator it;
    for (it = myList.begin(); it != myList.end();)
    {
        if (*it == 5)
        {
            it = myList.erase(it);
        }
        else
            ++it;
    }

    for (it = myList.begin(); it != myList.end(); ++it)
    {
        cout << *it<<endl;
    }
}
