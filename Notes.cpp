///item 1. Iterators
//find 4 error with:

int main()
{
    vector<Date> e;
    copy( istream_iterator<Date>( cin ), istream_iterator<Date>(), back_inserter( e ) ); //[WRONG] missing a while?
    vector<Date>::iterator first = find( e.begin(), e.end(), "01/01/95" );
    vector<Date>::iterator last = find( e.begin(), e.end(), "12/31/95" );
    *last = "12/30/95"; //doesn't check for end
    copy( first, last, ostream_iterator<Date>( cout, "\n" ) ); //doesn't check for first, or if last < first
    e.insert( --e.end(), TodaysDate() ); //[MISSED] --e.end() is illegal to modify temporaries of built-in types (debatable if error) : use e.insert( e.end() - 1, TodaysDate() ); 
    copy( first, last, ostream_iterator<Date>( cout, "\n" ) ); //[MISSED: iterators may become invalidated in vectors after insertion (could be moved)]
}

///item 2. Case Insensitive Strings Part 1
//1. Case insensitive means that for the purposes of equality a or A are the same

#include <string>
#include <algorithm>
#include <cctype>
#include <cassert>

//case insensitive string
class ci_string : public std::string
{
public:
    ci_string(const char* c) : std::string(c) {}

    bool operator==(const char* other)
    {
        return *this == ci_string(other);
    }

    bool operator==(const ci_string& other)
    {
        if (size() != other.size()) {
            return false;
        }
        return std::equal(begin(), end(), other.begin(), [&](const char& a, const char& b) {return std::tolower(a) == std::tolower(b); });
    }
};

int main()
{
    ci_string s("AbCdE");
    // case insensitive
    //
    assert(s == "abcde");
    assert(s == "ABCDE");
    // still case-preserving, of course
    //
    assert(strcmp(s.c_str(), "AbCdE") == 0);
    assert(strcmp(s.c_str(), "abcde") != 0);

    return 0;
}

//[ANALYSIS]: there is a better/similar way to approach this, as string isn't a class but a typedef that already has character comparison bits to it. here's the proposed solution is not inhereting from string but char_traits:

struct ci_char_traits : public std::char_traits<char>
    // just inherit all the other functions
    // that we don't need to replace
{
    static bool eq(char c1, char c2)
    {
        return toupper(c1) == toupper(c2);
    }
    static bool lt(char c1, char c2)
    {
        return toupper(c1) < toupper(c2);
    }
    static int compare(const char* s1,
        const char* s2,
        size_t n)
    {
        return memicmp(s1, s2, n);
    }
    // if available on your platform,
    // otherwise you can roll your own
    static const char*
        find(const char* s, int n, char a)
    {
        while (n-- > 0 && toupper(*s) != toupper(a))
        {
            ++s;
        }
        return n >= 0 ? s : 0;
    }
};

typedef basic_string<char, ci_char_traits> ci_string;


//3. is this a good approach?
// Not really, especially if you convert it out of that object class. it's more useful to keep it as part of a comparison function

///item 3. Case Insensitive Strings Part 2

//Consider:
struct ci_char_traits : public char_traits<char>
{
    static bool eq( char c1, char c2 ) { /*...*/ }
    static bool lt( char c1, char c2 ) { /*...*/ }
    static int compare( const char* s1,
    const char* s2,
    size_t n ) { /*...*/ }
    static const char*
    find( const char* s, int n, char a ) { /*...*/ }
};

//1. is this safe to inherit
// maybe? [CORRECTION]: Maybe but because this is traits inheritance, it does not intended to be used polymorphically.

//2
ci_string s = "abc";
cout << s << endl;
//Fails to compile as maybe we did not define operator= or operator<< for strings with the same char traits.

///Item 4. Maximally Reusable Generic Containers Part 1
// create a basic fixed array template
#include <string>
#include <algorithm>
#include <cctype>
#include <cassert>
#include <type_traits>
#include <iterator>

template<typename T, size_t size>
class fixed_vector
{
public:
    fixed_vector()
    {
        v_ = static_cast<T*>(malloc(sizeof(T) * size));
    }

    fixed_vector(const fixed_vector<T, size>& other)
    {
        if (*this == other)
        {
            return;
        }
        v_ = static_cast<T*>(malloc(sizeof(T) * size));
        for (int i = 0; i < size; ++i)
        {
            v_[i] = other.v_[i];
        }
    }

    fixed_vector& operator=(const fixed_vector<T, size>& other)
    {
        if (*this == other)
        {
            return;
        }
        v_ = malloc(sizeof(T) * size);
        for (int i = 0; i < size; ++i)
        {
            v_[i] = other.v_[i];
        }
    }

    bool operator==(const fixed_vector<T, size>& other)
    {
        return v_ == other.v_;
    }

    T& operator[](const size_t i)
    {
        return v_[i];
    }

    ~fixed_vector()
    {
        free(v_);
    }
    typedef T* iterator;
    typedef const T* const_iterator;
    iterator begin() { return v_; }
    iterator end() { return v_ + size; }
    const_iterator begin() const { return v_; }
    const_iterator end() const { return v_ + size; }
private:
    T* v_;
};



int main()
{
    int i = 3;
    fixed_vector<int, 2> o;
    o[0] = 1;
    o[1] = 4;
    o[2] = 52;


    fixed_vector<int, 2> p = o;

    fixed_vector<int, 2> s(o);
    return 0;
}

//critique the proposed solution:
template<typename T, size_t size>
class fixed_vector
{
public:
    typedef T* iterator;
    typedef const T* const_iterator;
    fixed_vector() { } //no allocation done
    template<typename O, size_t osize>
    fixed_vector(const fixed_vector<O, osize>& other) //no checks for different sizes, but ok will copy the minimum aparently.
    {
        //no checks if they are the same
        //no allocation (? maybe the v_[size] is enough ?
        //[MISSED] no strong exception wrapping, which is important on a constructor
        copy(other.begin(),
            other.begin() + min(size, osize),
            begin());
    }
    template<typename O, size_t osize>
    fixed_vector<T, size>&
        operator=(const fixed_vector<O, osize>& other) //same problem here
    {
        //no checks if they are the same
        copy(other.begin(),
            other.begin() + min(size, osize),
            begin());
        return *this;
    }
    //no destructor
    iterator begin() { return v_; }
    iterator end() { return v_ + size; }
    const_iterator begin() const { return v_; }
    const_iterator end() const { return v_ + size; }
private:
    T v_[size];
};

//things missed: default implementation probably worked fine.
// a copy constructor must be of exact same type (not really a miss)
//usability considerations: 
//      * maybe better to extend it so that it works with different types also
//as long as source type are assignable. 
//      * support varying sizes
//      * add copies that use iterators instead

// important: as you define a constructor of any kind, the compiler will not generate the default one for you

///Stopped at : Item 6 Temporary Objects
