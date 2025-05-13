## NativeLINQ ##

C++ LINQ library.  

This is the early pre-alpha version, so do not intend to use this software anyhow if you want to save your nervous!

Usage sample (for now):
	
```
#!c++

	auto v1 = 
		From(arr, arr + 5)->
		Aggregate([](int i, int j) {return i + j; });

	auto v2 =
		Where(arr, arr + 5, [](int i) {return i > 1; })->
		Average();

	auto v3 =
		Where(arr, arr + 5, [](int i) {return i > 1; })->
		ToVector();

	auto v4 =
		From(arr, arr + 5)->
		Where([](int i) {return i > 1; })->
		Select([](int i) {return i * 5; })->Count();
```

# License #
The MIT License (MIT)
