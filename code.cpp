#include <iostream>
#include <random>

void BubbleSort(int a[], int N)
{
	for (int i = 0; i < N - 1; ++i)
	{
		for (int j = 0; j < N - i - 1; ++j)
		{
			if (a[j] > a[j + 1])
				std::swap(a[j], a[j + 1]);
		}
	}
}

int Partition(int a[], int start, int end)
{
	int pivot = a[end];
	int i = start;
	for (int j = start; j <= end; ++j)
	{
		if (a[j] < pivot)
		{
			std::swap(a[i], a[j]);
			++i;
		}
	}
	std::swap(a[i], a[end]);
	return i;
}

void QuickSort(int a[], int start, int end)
{
	if (start < end)
	{
		int p = Partition(a, start, end);
		QuickSort(a, start, p - 1);
		QuickSort(a, p + 1, end);
	}
}

void InsertionSort(int a[], int count)
{
	for (size_t i = 1; i < count; ++i)
	{
		int j = i - 1;
		int current = a[i];
		while (j >= 0 && a[j] > current)
		{
			a[j + 1] = a[j];
			--j;
		}
		a[j + 1] = current;
	}
}

void BiSelectionSort(int a[], int n)
{
	for (size_t i = 0; i < n / 2; ++i)
	{
		int min = i;
		int max = n - i - 1;

		for (size_t j = i; j < n - i; ++j)
		{
			if (a[j] < a[min])
				min = j;
			if (a[j] > a[max])
				max = j;
		}
		std::swap(a[min], a[i]);
		if (i == max)
		{
			std::swap(a[min], a[n - i - 1]);
		}
		else
		{
			std::swap(a[max], a[n - i - 1]);
		}
		
	}
}

void Merge(int a[], int left, int middle, int right)
{
	size_t i = left, j = left, k = middle + 1;
	int* temp = new int[right + 1];

	while (j <= middle && k <= right)
	{
		if (a[j] <= a[k])
			temp[i++] = a[j++];
		else
			temp[i++] = a[k++];
	}
	while (j <= middle)
		temp[i++] = a[j++];
	while (k <= right)
		temp[i++] = a[k++];
	for (i = left; i <= right; ++i)
	{
		a[i] = temp[i];
	}
	delete[] temp;
}

void MergeSort(int a[], int left, int right)
{
	if (left < right)
	{
		unsigned const middle = (left + right) / 2;
		MergeSort(a, left, middle);
		MergeSort(a, middle + 1, right);
		Merge(a, left, middle, right);
	}
}

void PrintArray(int a[], int n)
{
	for (int i = 0; i < n; ++i)
		std::cout << a[i] << ", ";
}

int main()
{
	const int n = 20;
	int a[n] {0};
	std::random_device rd;	//Will be used to obtain a seed for the random number engine
	std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
	std::uniform_int_distribution<> distrib(1, 100);
	for (int i = 0; i < n; ++i)
		a[i] = distrib(gen);

	std::cout << "Unsorted Array: ";
	PrintArray(a, n);
	std::cout << std::endl;

	//BubbleSort(a, n);
	//BiSelectionSort(a, n);
	//InsertionSort(a, n);
	//QuickSort(a, 0, n - 1);
	MergeSort(a, 0, n - 1);
	
	std::cout << "Sorted Array: ";
	PrintArray(a, n);
	std::cout << std::endl;
}