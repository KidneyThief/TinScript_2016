Print("hello world");

void Heap::OnCreate() : CScriptObject
{
	Print("My heap!");

	int self.capacity = 100;
	int self.size = 0;
	int[100] self.heap;
}

int Heap::parent(int i)
{
	return (i - 1) / 2;
}

int Heap::left(int i)
{
	return (2 * i) + 1;
}

int Heap::right(int i)
{
	return (2 * i) + 2;
}

void Heap::clear()
{
	self.size = 0;
}

void Heap::insertKey(int k)
{
	if (self.size >= self.capacity)
	{
		Print("### ERROR - Heap full");
	}

	self.size++;
	int i = self.size - 1;
	self.heap[i] = k;

	while (i != 0 && self.heap[self.parent(i)] > self.heap[i])
	{
		self.swap(i, self.parent(i));
		i = self.parent(i);
	}
}

void Heap::getMin()
{
	if (self.size <= 0)
	{
		return -1;
	}

	return self.heap[0];
}

void Heap::extractMin()
{
	if (self.size <= 0)
	{
		return -1;
	}

	if (self.size == 1)
	{
		self.size--;
		return self.heap[0];
	}

	int root = self.heap[0];
	self.heap[0] = self.heap[self.size -1];
	self.size--;
	self.MinHeapify(0);

	return root;
}

void Heap::MinHeapify(int i)
{
	int l = self.left(i);
	int r = self.right(i);
	int smallest = i;
	int smallest_val = -1;
	if (l < self.size && self.heap[l] < self.heap[i])
	{
		smallest = l;
		smallest_val = self.heap[l];
	}
	if (r < self.size && self.heap[r] < self.heap[smallest])
	{
		if (smallest_val >= 0 && self.heap[r] < smallest_val)
		{
			smallest = r;
		}
	}

	if (smallest != i)
	{
		self.swap(i, smallest);
		self.MinHeapify(smallest);
	}
}

void Heap::swap(int x, int y)
{
	int temp = self.heap[x];
	self.heap[x] = self.heap[y];
	self.heap[y] = temp;
}

void Heap::test()
{
	self.clear();
	self.insertKey(3);
	self.insertKey(2);
	self.insertKey(4);
	self.insertKey(1);
	self.insertKey(5);
	self.insertKey(9);

	while (self.size > 0)
	{
		int val = self.extractMin();
		Print(val);
	}
}

