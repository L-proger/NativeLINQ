#include <memory>
#include <iostream>
#include <functional>
#include <algorithm>
#include <vector>

template<typename TResult>
struct IEnumerator;

template<typename TResult>
struct IEnumerable;

template<typename TSource>
struct ReverseIterator;

template<typename TResult>
class Iterator;

template<typename TResult>
struct LinqForwardIterator {
	std::shared_ptr<IEnumerator<TResult>> Target;
	bool isBegin;
	bool isEnd;

	TResult& operator*() const {
		return Target->Current();
	}

	LinqForwardIterator& operator++() {
		isBegin = false;
		isEnd = !Target->MoveNext();
		return *this;
	}

	bool operator==(LinqForwardIterator const & rhs) const {
		return (isBegin == rhs.isBegin) && (isEnd == rhs.isEnd) && (Target == rhs.Target);
	}

	bool operator!=(LinqForwardIterator const & rhs) const {
		return (isBegin != rhs.isBegin) || (isEnd != rhs.isEnd) || (Target != rhs.Target);
	}
};

template<typename TResult>
struct IEnumerator {
	using iterator_type = LinqForwardIterator<TResult>;
	virtual TResult& Current() = 0;
	virtual bool MoveNext() = 0;
	virtual void Reset() = 0;
	virtual std::shared_ptr<IEnumerator<TResult>> AsEnumerator() = 0;

	iterator_type begin() {
		Reset();

		bool failed = !MoveNext();
		iterator_type itr;
		itr.Target = AsEnumerator();
		if(failed) {
			itr.isBegin = false;
			itr.isEnd = true;
		} else {
			itr.isBegin = true;
			itr.isEnd = false;
		}
		return itr;
	}

	iterator_type end() {
		iterator_type itr;
		itr.Target = AsEnumerator();
		itr.isBegin = false;
		itr.isEnd = true;
		return itr;
	}

	std::vector<TResult> ToVector() {
		std::vector<TResult> result;
		Reset();
		while(MoveNext()) {
			result.push_back(Current());
		}
		return result;
	}

	std::shared_ptr<Iterator<TResult>> Reverse() {
		return std::shared_ptr<Iterator<TResult>>(new ReverseIterator<TResult>(AsEnumerator()));
	}

	TResult Sum() {
		TResult result = 0;
		Reset();
		while(MoveNext()) {
			result += Current();
		}
		return result;
	}

	TResult Max() {
		bool hasValue = false;
		TResult value;
		Reset();
		while(MoveNext()) {
			if(hasValue) {
				if(Current() > value) {
					value = Current();
				}
			} else {
				hasValue = true;
				value = Current();
			}
		}
		return value;
	}

	TResult Min() {
		bool hasValue = false;
		TResult value;
		Reset();
		while(MoveNext()) {
			if(hasValue) {
				if(Current() < value) {
					value = Current();
				}
			} else {
				hasValue = true;
				value = Current();
			}
		}
		return value;
	}

	size_t Count() {
		size_t result = 0;
		Reset();
		while(MoveNext()) {
			result++;
		}
		return result;
	}

	double Average() {
		bool hasValue = false;
		TResult value;
		size_t count = 0;
		Reset();
		while(MoveNext()) {
			count++;
			if(hasValue) {
				value += Current();
			} else {
				hasValue = true;
				value = Current();
			}
		}
		return (double)value / (double)count;
	}

	TResult Aggregate(std::function<TResult(TResult,TResult)> func) {
		bool val = false;
		TResult tmp;
		Reset();
		while(MoveNext()) {
			if(!val) {
				tmp = Current();
				val = true;
				continue;
			}
			tmp = func(tmp, Current());
		}
		return tmp;
	}



	bool Any() {
		while(MoveNext()) {
			return true;
		}
		return false;
	}
};

template <typename T>
struct function_traits : public function_traits<decltype(&T::operator())> {};

template <typename ClassType, typename ReturnType, typename... Args>
struct function_traits<ReturnType(ClassType::*)(Args...) const> {
	enum { arity = sizeof...(Args) };
	typedef ReturnType result_type;
	

	template <size_t i>
	struct arg {
		typedef typename std::tuple_element<i, std::tuple<Args...>>::type type;
	};

	typedef typename std::tuple_element<0, std::tuple<Args...>>::type Arg0;

	typedef std::function<result_type(typename arg<0>::type)> func;
};

template<typename TResult>
struct IEnumerable {
	virtual std::shared_ptr<IEnumerator<TResult>> GetEnumerator() = 0;
};

template<typename TSource>
static std::function<bool(TSource)> CombinePredicates(std::function<bool(TSource)> predicate1, std::function<bool(TSource)> predicate2) {
	return [=](TSource x) {return predicate1(x) && predicate2(x); };
}

template<typename TSelector>
struct SelectorDesc {
	typedef typename function_traits<TSelector>::func func;
	typedef typename function_traits<TSelector>::Arg0 TArg;
	typedef typename function_traits<TSelector>::result_type TResult;
};

template<typename Tselector1, typename Tselector2>
struct SelectorCombiner {
	typedef typename SelectorDesc<Tselector1> d1;
	typedef typename SelectorDesc<Tselector2> d2;
	typedef typename d2::TResult TResult;
	typedef typename d1::TArg TSource;
	typedef std::function<TResult(TSource)> func;
};

template<typename Tselector1, typename Tselector2>
static auto CombineSelectors(Tselector1 selector1, Tselector2 selector2)-> typename SelectorCombiner<Tselector1, Tselector2>::func{
	return [=](typename SelectorCombiner<Tselector1, Tselector2>::TSource x) {return selector2(selector1(x)); };
}

template<typename TResult>
class Iterator : public IEnumerable<TResult>, public IEnumerator<TResult>, public std::enable_shared_from_this<Iterator<TResult>> {
public:
	
	TResult _currentValue;

	Iterator()
		: _resetted(true)
	{

	}


	std::shared_ptr<IEnumerator<TResult>> GetEnumerator() override final {
		return this->shared_from_this();
	}

	std::shared_ptr<IEnumerator<TResult>> AsEnumerator() override final {
		return this->shared_from_this();
	}

	TResult First(std::function<bool(TResult)> predicate) {
		Reset();
		while(MoveNext()) {
			if(predicate(Current())) {
				return Current();
			}
		}
		throw std::exception("LINQ: Failed to get first element");
	}

	TResult First() {
		Reset();
		while(MoveNext()) {
			return Current();
		}
		throw std::exception("LINQ: Failed to get first element");
	}

	TResult Last(std::function<bool(TResult)> predicate) {
		TResult result;
		bool found = false;

		Reset();
		while(MoveNext()) {
			if(predicate(Current())) {
				result = Current();
				found = true;
			}
		}

		if(found) return result;
		throw new std::exception("LINQ: failed to find last");
	}

	TResult Last() {
		//if(source == null) throw Error.ArgumentNull("source");
		//if(predicate == null) throw Error.ArgumentNull("predicate");
		TResult result;
		bool found = false;
		Reset();
		while(MoveNext()) {
			result = Current();
			found = true;
		}
		if(found) return result;
		throw new std::exception("LINQ: failed to find last");
	}

	TResult& Current() override {
		return _currentValue;
	}

	void Reset() override {
		_resetted = true;
	}
protected:
	bool _resetted;
};

template<typename TSource>
struct WhereEnumerableIterator;

template<typename TSource, typename TResult>
struct WhereSelectEnumerableIterator : public Iterator<TResult>{
	using TPredicate = std::function<bool(TSource)>;
	using TSelector = std::function<TResult(TSource)>;
	using TSourcePtr = std::shared_ptr<IEnumerable<TSource>>;
	TSourcePtr _source;
	TPredicate _predicate;
	TSelector _selector;

	WhereSelectEnumerableIterator(TSourcePtr source, TPredicate predicate, TSelector selector)
		:_source(source), _predicate(predicate), _selector(selector)
	{
	}

	bool MoveNext() override {
		auto enumerator = _source->GetEnumerator();
		while(enumerator->MoveNext()) {
			auto item = enumerator->Current();
			if(_predicate == nullptr || _predicate(item)) {
				_currentValue = _selector(item);
				return true;
			}
		}
		return false;
	}

	template<typename TSelector>
	using DerivedIterator = WhereSelectEnumerableIterator<TSource, typename function_traits<TSelector>::result_type>;

	template<typename TSelector>
	using DerivedIteratorPtr = std::shared_ptr<DerivedIterator<TSelector>>;
	
	template<typename TSelector2>
	auto Select(TSelector2 selector)->DerivedIteratorPtr<TSelector2> {
		return DerivedIteratorPtr(new DerivedIterator(_source, _predicate, CombineSelectors(_selector, selector)));
	}

	std::shared_ptr<WhereEnumerableIterator<TResult>> Where(std::function<bool(TResult)> predicate) {
		return WhereEnumerableIterator<TResult>(new WhereEnumerableIterator<TResult>(shared_from_this(), predicate));
	}

	void Reset() override {
		Iterator<TResult>::Reset();
		if(_source != nullptr) {
			_source->GetEnumerator()->Reset();
		}
	}
};

template<typename TSource>
struct WhereEnumerableIterator : public Iterator<TSource> {
	using TSourcePtr = std::shared_ptr<IEnumerable<TSource>>;

	TSourcePtr _source;
	std::function<bool(TSource)> _predicate;

	WhereEnumerableIterator(TSourcePtr source, std::function<bool(TSource)> predicate)
		:_source(source), _predicate(predicate) {}

	bool MoveNext() override {

		auto enumerator = _source->GetEnumerator();
	
		while(enumerator->MoveNext()) {
			auto item = enumerator->Current();
			if(_predicate(item)) {
				_currentValue = item;
				return true;
			}
		}
		return false;
	}


	template<typename TSelector>
	using DerivedIterator = WhereSelectEnumerableIterator<TSource, typename function_traits<TSelector>::result_type>;

	template<typename TSelector>
	using DerivedIteratorPtr = std::shared_ptr<DerivedIterator<TSelector>>;

	template<typename TSelector2>
	auto Select(TSelector2 selector)->DerivedIteratorPtr<TSelector2> {
		return DerivedIteratorPtr<TSelector2>(new DerivedIterator<TSelector2>(_source, _predicate, selector));
	}

	std::shared_ptr<WhereEnumerableIterator<TSource>> Where(std::function<bool(TSource)> predicate) {
		return std::shared_ptr<WhereEnumerableIterator<TSource>>(new WhereEnumerableIterator<TSource>(_source, CombinePredicates(_predicate, predicate)));
	}

	void Reset() override {
		Iterator<TSource>::Reset();
		if(_source != nullptr) {
			_source->GetEnumerator()->Reset();
		}
	}
};

template<typename TSourceIter, typename TResult>
struct WhereSelectIterator : public Iterator<TResult>{
	typedef typename std::iterator_traits<TSourceIter>::value_type TSource;
	TSourceIter _begin;
	TSourceIter _end;
	TSourceIter _current;

	using TPredicate = std::function<bool(TSource)>;
	using TSelector = std::function<TResult(TSource)>;
	TPredicate _predicate;
	TSelector _selector;

	WhereSelectIterator(TSourceIter begin, TSourceIter end, TPredicate predicate, TSelector selector)
		:_predicate(predicate), _selector(selector), _begin(begin), _end(end), _current(end){

	}

	template<typename TSelector>
	using DerivedIterator = WhereSelectIterator<TSourceIter, typename function_traits<TSelector>::result_type>;

	template<typename TSelector>
	using DerivedIteratorPtr = std::shared_ptr<DerivedIterator<TSelector>>;

	template<typename TSelector>
	auto Select(TSelector selector)->DerivedIteratorPtr<TSelector> {
		return DerivedIteratorPtr<TSelector>(new DerivedIterator<TSelector>
			(_begin, _end, _predicate, CombineSelectors(_selector, selector)));
	}

	std::shared_ptr<WhereEnumerableIterator<TResult>> Where(std::function<bool(TResult)> predicate) {
		return std::shared_ptr<WhereEnumerableIterator<TResult>>(new WhereEnumerableIterator<TResult>(shared_from_this(), predicate));
	}

	bool MoveNext() override {
		while(true) {
			//step next value
			if(_resetted) {
				_current = _begin;
				_resetted = false;
			} else {
				if(_current == _end) {
					return false;
				}
				++_current;
				if(_current == _end) {
					return false;
				}
			}

			if(_predicate == nullptr || _predicate(*_current)) {
				_currentValue = _selector(*_current);
				return true;
			}
		}
		return false;
	}
};

template<typename TSourceIter>
struct WhereIterator : public Iterator<typename std::iterator_traits<TSourceIter>::value_type>
{
	using TSource = typename std::iterator_traits<TSourceIter>::value_type;
	typedef typename std::iterator_traits<TSourceIter>::value_type TSource;
	TSourceIter _begin;
	TSourceIter _end;
	TSourceIter _current;

	using TPredicate = std::function<bool(TSource)>;
	TPredicate _predicate;

	WhereIterator(TSourceIter begin, TSourceIter end, TPredicate predicate)
		:_begin(begin), _end(end), _predicate(predicate)
	{
	}


	bool MoveNext() override {
		while(true) {
			//step next value
			if(this->_resetted) {
				_current = _begin;
				this->_resetted = false;
			} else {
				if(_current == _end) {
					return false;
				}
				++_current;
				if(_current == _end) {
					return false;
				}
			}

			auto& item = *_current;
			if(_predicate(item)) {
				this->_currentValue = item;
				return true;
			}
		}
		return false;
	}


	template<typename TSelector>
	using DerivedIterator = WhereSelectIterator<TSourceIter, typename function_traits<TSelector>::result_type>;

	template<typename TSelector>
	using DerivedIteratorPtr = std::shared_ptr<DerivedIterator<TSelector>>;



	template<typename TSelector>
	auto Select(TSelector selector) -> DerivedIteratorPtr<TSelector> {
		return DerivedIteratorPtr<TSelector>(new DerivedIterator<TSelector>(_begin, _end, _predicate, selector));
	}
	
	std::shared_ptr<WhereIterator<TSource>> Where(std::function<bool(TSource)> predicate) {
		return std::shared_ptr<WhereIterator<TSource>>(new WhereIterator<TSource>(_begin, _end, CombinePredicates(_predicate, predicate)));
	}
};

template<typename T>
struct DefaultComparer {
	static int Compare(const T& left, const T& right) {
		if(left > right) {
			return 1;
		} else if(left < right) {
			return -1;
		} else {
			return 0;
		}
	}
};

template<typename TSource>
struct ReverseIterator : public Iterator<TSource> {
	using TSourceContainer = IEnumerator<TSource>;
	using TSourceContainerPtr = std::shared_ptr<IEnumerator<TSource>>;
	using TBuffer = std::vector<TSource>;
	using TIterator = typename std::vector<TSource>::reverse_iterator;
	TIterator _currentIterator;

	TSourceContainerPtr _source;
	TBuffer _buffer;



	ReverseIterator(TSourceContainerPtr source)
	:_source(source) {}

	bool MoveNext() override {
		if(_resetted) {
			while(_source->MoveNext()) {
				_buffer.push_back(_source->Current());
			}
			_currentIterator = _buffer.rbegin();
			_resetted = false;
		} else {
			if(_currentIterator == _buffer.rend()) {
				return false;
			}
			++_currentIterator;
			if(_currentIterator == _buffer.rend()) {
				return false;
			}
		}
		_currentValue = *_currentIterator;
		return true;
	}

	void Reset() override {
		Iterator<TSource>::Reset();
		if(_source != nullptr) {
			_source->Reset();
			_buffer.clear();
		}
	}
};

template<typename TSourceIter, typename TSelector>
static auto Select(TSourceIter begin, TSourceIter end, TSelector selector = nullptr)
->std::shared_ptr<WhereSelectIterator<TSourceIter, typename function_traits<TSelector>::result_type>>{
	using Iter = WhereSelectIterator<TSourceIter, typename function_traits<TSelector>::result_type>;
	using IterPtr = std::shared_ptr<Iter>;
	return IterPtr(new Iter(begin, end, nullptr, selector));
}


template<typename TSource, typename TSelector>
static auto Select(TSource source, TSelector selector = nullptr)
->std::shared_ptr<WhereSelectIterator<typename TSource::iterator, typename function_traits<TSelector>::result_type>> {
	using Iter = WhereSelectIterator<typename TSource::iterator, typename function_traits<TSelector>::result_type>;
	using IterPtr = std::shared_ptr<Iter>;
	return IterPtr(new Iter(source.begin(), source.end(), nullptr, selector));
}


template<typename TSourceIter>
static auto From(TSourceIter begin, TSourceIter end)
->std::shared_ptr<WhereSelectIterator<TSourceIter, typename std::iterator_traits<TSourceIter>::value_type>> {
	using Iter = WhereSelectIterator<TSourceIter, typename std::iterator_traits<TSourceIter>::value_type>;
	using IterPtr = std::shared_ptr<Iter>;
	return IterPtr(new Iter(begin, end, nullptr, [](typename std::iterator_traits<TSourceIter>::value_type v) {return v; }));
}

template<typename TSourceIter>
static std::shared_ptr<WhereIterator<TSourceIter>> Where(TSourceIter begin, TSourceIter end, std::function<bool(typename std::iterator_traits<TSourceIter>::value_type)> predicate) {
	return std::shared_ptr<WhereIterator<TSourceIter>>(new WhereIterator<TSourceIter>(begin, end, predicate));
}

template<typename TSource>
static auto From(TSource& source)->std::shared_ptr<WhereSelectIterator<typename TSource::iterator, typename std::iterator_traits<typename TSource::iterator>::value_type>> {
	using Iter = WhereSelectIterator<typename TSource::iterator, typename std::iterator_traits<typename TSource::iterator>::value_type>;
	using IterPtr = std::shared_ptr<Iter>;
	return IterPtr(new Iter(source.begin(), source.end(), nullptr, [](typename std::iterator_traits<typename TSource::iterator>::value_type v) {return v; }));
}

template<typename TSource>
static std::shared_ptr<WhereIterator<typename TSource::iterator>> Where(TSource& source, std::function<bool(typename std::iterator_traits<typename TSource::iterator>::value_type)> predicate) {
	return std::shared_ptr<WhereIterator<typename TSource::iterator>>(new WhereIterator<typename TSource::iterator>(source.begin(), source.end(), predicate));
}
