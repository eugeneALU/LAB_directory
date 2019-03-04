/**************************
** TDDD56 Lab 3
***************************
** Author:
** August Ernstsson
**************************/

#include <iostream>

#include <skepu2.hpp>

/* SkePU user functions */
float product(float x, float y)
{
	return  x * y;
}

float sum(float x, float y)
{
	return x + y;
}


int main(int argc, const char* argv[])
{
	if (argc < 2)
	{
		std::cout << "Usage: " << argv[0] << " <input size> <backend>\n";
		exit(1);
	}

	const size_t size = std::stoul(argv[1]);
	auto spec = skepu2::BackendSpec{skepu2::Backend::typeFromString(argv[2])};
    spec.setCPUThreads(12);


	/* Skeleton instances */
	auto prod = skepu2::Map<2>(product);
	auto sumup = skepu2::Reduce(sum);

	auto combine = skepu2::MapReduce<2>(product,sum);

	/* Set backend (important, do for all instances!) */
	prod.setBackend(spec);
	sumup.setBackend(spec);
	combine.setBackend(spec);

	/* SkePU containers */
	skepu2::Vector<float> v1(size, 1.0f), v2(size, 2.0f);
	skepu2::Vector<float> result(v1.size());


	/* Compute and measure time */
	float resComb, resSep;

	auto timeComb = skepu2::benchmark::measureExecTime([&]
	{
			resComb = combine(v1, v2);
	});

	auto timeSep = skepu2::benchmark::measureExecTime([&]
	{
			prod(result, v1, v2);
			resSep = sumup(result);
	});

	std::cout << "Time Combined: " << (timeComb.count() / 10E6) << " seconds.\n";
	std::cout << "Time Separate: " << ( timeSep.count() / 10E6) << " seconds.\n";


	std::cout << "Result Combined: " << resComb << "\n";
	std::cout << "Result Separate: " << resSep  << "\n";
	//for check
	//std::cout << "V1: " << v1 << "\n";
	//std::cout << "V2: " << v2  << "\n";

	return 0;
}
