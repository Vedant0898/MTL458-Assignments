#include <iostream>
#include <unordered_map>
#include <string>

using namespace std;

/**
 * Node of Doubly Linked List
 */
struct DLLNode
{
    uint32_t data;
    DLLNode *next;
    DLLNode *prev;
    DLLNode(uint32_t data) : data(data), next(NULL), prev(NULL) {}
};

/**
 * Node of Heap
 */
struct HeapNode
{
    uint32_t key;
    uint32_t value;
    HeapNode() : key(0), value(0) {}                                   // default constructor
    HeapNode(uint32_t key, uint32_t value) : key(key), value(value) {} // parameterized constructor
};

/**
 * FIFO Cache Implementation
 */
class FIFOCache
{
private:
    uint32_t *arr;
    uint32_t size;
    uint32_t capacity;
    uint32_t front;
    uint32_t rear;
    unordered_map<uint32_t, uint32_t> count; // using as set to check if element exist in cache

    /**
     * Evict element from Cache according to FIFO policy
     */
    void evictElementFromCache()
    {
        // remove first element
        if (size == 0)
            return;
        count.erase(arr[front]);
        front = (front + 1) % capacity;
        size--;
    }

public:
    /**
     * Constructor
     */
    FIFOCache(uint32_t capacity) : capacity(capacity), size(0), front(0), rear(0)
    {
        arr = new uint32_t[capacity];
        count.clear();
    }

    /**
     * Check if element is in Cache
     */
    bool checkInCache(uint32_t data)
    {
        return count.find(data) != count.end();
    }

    /**
     * Insert element in Cache
     */
    void insertElementInCache(uint32_t data)
    {
        if (size == capacity)
        {
            evictElementFromCache();
        }
        arr[rear] = data;
        rear = (rear + 1) % capacity;
        size++;
        count[data] = 1;
    }

    /**
     * Destructor
     */
    ~FIFOCache()
    {
        delete[] arr;
    }
};

/**
 * LIFO Cache Implementation
 */
class LIFOCache
{
private:
    uint32_t *arr;
    uint32_t size;
    uint32_t capacity;
    unordered_map<uint32_t, uint32_t> count; // using as set to check if element exist in cache

    /**
     * Evict element from Cache according to LIFO policy
     */
    void evictElementFromCache()
    {
        // remove last element
        if (size == 0)
            return;
        count.erase(arr[size - 1]);
        size--;
    }

public:
    /**
     * Constructor
     */
    LIFOCache(uint32_t capacity) : capacity(capacity), size(0)
    {
        arr = new uint32_t[capacity];
        count.clear();
    }

    /**
     * Check if element is in Cache
     */
    bool checkInCache(uint32_t data)
    {
        return count.find(data) != count.end();
    }

    /**
     * Insert element in Cache
     */
    void insertElementInCache(uint32_t data)
    {
        if (size == capacity)
        {
            evictElementFromCache();
        }
        arr[size] = data;
        size++;
        count[data] = 1;
    }

    /**
     * Destructor
     */
    ~LIFOCache()
    {
        delete[] arr;
    }
};

/**
 * LRU Cache Implementation
 */
class LRUCache
{
private:
    DLLNode *front; // placeholder node for front
    DLLNode *rear;  // placeholder node for rear
    uint32_t size;
    uint32_t capacity;
    unordered_map<uint32_t, DLLNode *> mp;

    /**
     * Insert node at front of DLL
     */
    void insertAtFront(DLLNode *node)
    {
        node->next = front->next;
        node->prev = front;
        front->next->prev = node;
        front->next = node;
        size++;
    }

    /**
     * Delete node from DLL
     */
    void deleteNode(DLLNode *node)
    {
        node->prev->next = node->next;
        node->next->prev = node->prev;
        size--;
    }

    /**
     * Delete node from rear of DLL
     */
    DLLNode *deleteAtRear()
    {
        DLLNode *node = rear->prev;
        deleteNode(node);
        return node;
    }

    /**
     * Evict element from Cache according to LRU policy
     */
    void evictElementFromCache()
    {
        if (size == 0)
            return;
        DLLNode *node = deleteAtRear();
        mp.erase(node->data);
        delete node;
    }

public:
    /**
     * Constructor
     */
    LRUCache(uint32_t capacity) : capacity(capacity), size(0)
    {
        front = new DLLNode(-1);
        rear = new DLLNode(-1);
        front->next = rear;
        rear->prev = front;
        mp.clear();
    }

    /**
     * Check if element is in Cache
     */
    bool checkInCache(uint32_t data)
    {
        if (mp.find(data) == mp.end())
            return false;
        DLLNode *node = mp[data];
        deleteNode(node);
        insertAtFront(node);
        return true;
    }

    /**
     * Insert element in Cache
     */
    void insertElementInCache(uint32_t data)
    {
        if (size == capacity)
        {
            evictElementFromCache();
        }
        DLLNode *node = new DLLNode(data);
        insertAtFront(node);
        mp[data] = node;
    }

    /**
     * Destructor
     */
    ~LRUCache()
    {
        while (front != NULL)
        {
            DLLNode *temp = front;
            front = front->next;
            delete temp;
        }
    }
};

/**
 * Optimal Cache Implementation
 */
class OptimalCache
{
private:
    HeapNode *arr; // array representation of heap
    uint32_t capacity;
    uint32_t size;
    unordered_map<uint32_t, uint32_t> mp; // to store index of element in heap array

    /**
     * Upheap operation
     */
    void upHeap(int idx)
    {
        while (idx > 0)
        {
            int parent = (idx - 1) / 2;
            if (arr[parent].key < arr[idx].key)
            {
                swap(arr[parent], arr[idx]);
                mp[arr[parent].value] = parent;
                mp[arr[idx].value] = idx;
                idx = parent;
            }
            else
            {
                break;
            }
        }
    }

    /**
     * Downheap operation
     */
    void downHeap(int idx)
    {
        while (2 * idx + 1 < size)
        {
            int left = 2 * idx + 1;
            int right = 2 * idx + 2;
            int largest = idx;
            if (left < size && arr[left].key > arr[largest].key)
            {
                largest = left;
            }
            if (right < size && arr[right].key > arr[largest].key)
            {
                largest = right;
            }
            if (largest != idx)
            {
                swap(arr[largest], arr[idx]);
                mp[arr[largest].value] = largest;
                mp[arr[idx].value] = idx;
                idx = largest;
            }
            else
            {
                break;
            }
        }
    }

    /**
     * Evict element from Cache according to Optimal policy
     */
    void eviceElementFromCache()
    {
        if (size == 0)
            return;
        mp.erase(arr[0].value);
        size--;
        arr[0] = arr[size];
        mp[arr[0].value] = 0;
        downHeap(0);
    }

public:
    /**
     * Constructor
     */
    OptimalCache(uint32_t capacity) : capacity(capacity), size(0)
    {
        arr = new HeapNode[capacity];
        mp.clear();
    }

    /**
     * Check if element is in Cache
     */
    bool checkInCache(uint32_t data)
    {
        return mp.find(data) != mp.end();
    }

    /**
     * Insert element in Cache
     */
    void insertElementInCache(uint32_t key, uint32_t value)
    {
        if (size == capacity)
        {
            eviceElementFromCache();
        }
        arr[size] = HeapNode(key, value);
        mp[value] = size;
        upHeap(size);
        size++;
    }

    /**
     * Modify key of element in Cache
     */
    void modifyKey(uint32_t value, uint32_t key)
    {
        if (mp.find(value) == mp.end())
            return;
        int idx = mp[value];
        arr[idx].key = key;
        upHeap(idx);
        downHeap(idx);
    }

    /**
     * Destructor
     */
    ~OptimalCache()
    {
        delete[] arr;
    }
};

/**
 * Returns true if n is power of 2
 */
bool isPowerOfTwo(uint32_t n)
{
    return (n & (n - 1)) == 0;
}

/**
 * Return which power of 2 is n
 */
uint32_t getPowerOfTwo(uint32_t n)
{
    uint32_t p = 0;
    while (n > 1)
    {
        n = n >> 1;
        p++;
    }
    return p;
}

/**
 * Parse Hexadecimal string address to uint32_t
 */
uint32_t parseHex(string &s)
{
    uint32_t ans = 0;
    for (char c : s)
    {
        uint32_t t = c - '0';
        if (t >= 10)
        {
            t = c - 'A' + 10;
        }
        ans = ans * 16 + t;
    }
    return ans;
}

/**
 * Get Virtual Page Number
 */
uint32_t getVirtualPageNumber(uint32_t addr, uint32_t S, uint32_t P)
{
    return (addr & (S - 1)) >> P;
}

/**
 * Simulation for FIFO Cache
 */
void FIFO(uint32_t *M, uint32_t N, uint32_t S, uint32_t P, uint32_t K)
{
    FIFOCache cache(K);
    uint32_t cacheHit = 0;
    for (int i = 0; i < N; i++)
    {
        uint32_t vpn = getVirtualPageNumber(M[i], S, P);
        if (cache.checkInCache(vpn))
        {
            cacheHit++;
        }
        else
        {
            cache.insertElementInCache(vpn);
        }
    }
    cout << cacheHit << " ";
}

/**
 * Simulation for LIFO Cache
 */
void LIFO(uint32_t *M, uint32_t N, uint32_t S, uint32_t P, uint32_t K)
{
    LIFOCache cache(K);
    uint32_t cacheHit = 0;
    for (int i = 0; i < N; i++)
    {
        uint32_t vpn = getVirtualPageNumber(M[i], S, P);
        if (cache.checkInCache(vpn))
        {
            cacheHit++;
        }
        else
        {
            cache.insertElementInCache(vpn);
        }
    }
    cout << cacheHit << " ";
}

/**
 * Simulation for LRU Cache
 */
void LRU(uint32_t *M, uint32_t N, uint32_t S, uint32_t P, uint32_t K)
{
    LRUCache cache(K);
    uint32_t cacheHit = 0;
    for (int i = 0; i < N; i++)
    {
        uint32_t vpn = getVirtualPageNumber(M[i], S, P);
        if (cache.checkInCache(vpn))
        {
            cacheHit++;
        }
        else
        {
            cache.insertElementInCache(vpn);
        }
    }
    cout << cacheHit << " ";
}

/**
 * Simulation for Optimal Cache
 */
void Optimal(uint32_t *M, uint32_t N, uint32_t S, uint32_t P, uint32_t K)
{
    // Compute next occurrence of each element
    uint32_t *nextOccurrence = new uint32_t[N];
    unordered_map<uint32_t, uint32_t> mp;
    mp.reserve(N);
    const uint32_t INF = 1e9;
    for (int i = N - 1; i >= 0; i--)
    {
        uint32_t vpn = getVirtualPageNumber(M[i], S, P);
        if (mp.find(vpn) == mp.end())
        {
            nextOccurrence[i] = INF;
        }
        else
        {
            nextOccurrence[i] = mp[vpn];
        }
        mp[vpn] = i;
    }

    // Simulation for Optimal Cache
    OptimalCache heap(K);
    uint32_t cacheHit = 0;
    for (int i = 0; i < N; i++)
    {
        uint32_t vpn = getVirtualPageNumber(M[i], S, P);
        if (heap.checkInCache(vpn))
        {
            cacheHit++;
            heap.modifyKey(vpn, nextOccurrence[i]);
        }
        else
        {
            heap.insertElementInCache(nextOccurrence[i], vpn);
        }
    }
    // delete[] nextOccurrence;
    cout << cacheHit << "\n";
}

/**
 * Solve each test case
 */
void solve()
{
    // Read input
    uint32_t S, P, K;
    cin >> S >> P >> K;

    S = S << 20; // S = S * 2^20 (S in MB)
    P = P << 10; // P = P * 2^10 (P in KB)
    uint32_t N;
    cin >> N;
    uint32_t *M = new uint32_t[N]; // allocate array of length N on heap
    for (int i = 0; i < N; i++)
    {
        string addrHex;
        cin >> addrHex;
        M[i] = parseHex(addrHex);
    }

    // check S and P are power of 2
    if (!isPowerOfTwo(S))
    {
        cout << "Invalid S = " << (S >> 20) << "\n";
        return;
    }
    if (!isPowerOfTwo(P))
    {
        cout << "Invalid P = " << (P >> 10) << "\n";
        return;
    }
    // check K is valid
    if (K <= 0)
    {
        cout << "Invalid K = " << K << "\n";
        return;
    }

    P = getPowerOfTwo(P);

    FIFO(M, N, S, P, K);
    LIFO(M, N, S, P, K);
    LRU(M, N, S, P, K);
    Optimal(M, N, S, P, K);

    delete[] M;
}

/**
 * Main function
 */
int main()
{
    int T;
    cin >> T;
    for (int i = 0; i < T; i++)
    {
        solve();
    }
}