// fcalc.cpp
//

#include <stdexcept>
#include <iostream>
#include <deque>
#include <string>
#include <sstream>
#include <map>
#include <stdexcept>

using namespace std;

// types
typedef double Real;
typedef std::deque<Real> Stack;
struct Environment;
typedef void (*funcptr)(Environment&);
struct Atom {
	Atom () { action = nullptr; }
	Atom (funcptr a) { action = a; }
	funcptr action;
	std::deque<std::string> tail;
};
typedef std::map<std::string, Atom*> Lookup;
struct Environment {
	Environment (std::istream* input, std::ostream* output) {
		in = input;	out = output;
	}
	virtual ~Environment () {
		for (Lookup::iterator it = lookup.begin (); it != lookup.end (); ++it) {
			delete it->second;
		}
	}
	Lookup lookup;
	Stack stack;
	std::deque<std::string> tokens;
	std::istream* in;
	std::ostream* out;
};

// parsing
bool is_number (const char* s) {
	std::istringstream iss (s);
	Real dummy;
	iss >> std::noskipws >> dummy;
	return iss && iss.eof ();
}
void tokenize (std::string line, std::deque<std::string>& tokens) {
	std::stringstream vals;
	vals << line;
	while (!vals.eof ()) {
		std::string token;
		vals >> token;
		if (token.size ()) { tokens.push_back (token); }
	}
}

// functors
void eval (Environment& env) {
tail_call:
	for (size_t i = 0; i < env.tokens.size (); i++) {
		std::string t = env.tokens[i];
		std::cout << "__ " << t << endl;
		Lookup::iterator it = env.lookup.find (t);
		if (is_number(t.c_str ())) {
			env.stack.push_back (atof (t.c_str ()));
		} else {
			if (it == env.lookup.end ()) {
				std::stringstream msg;
				msg << "error: invalid identifier " << t;
				throw std::runtime_error (msg.str ());
			} else {
				if (it->second->action != nullptr) it->second->action (env);
				else {
					// FIXME: push_front della tail?
					env.tokens = it->second->tail;
					goto tail_call;
				}
			}
		}
	}
}
void comment (Environment& env) {
	env.tokens.clear ();
}
void define (Environment& env) {
	env.tokens.pop_front (); // remove :
	std::string t = env.tokens.front (); env.tokens.pop_front (); // remove W
	cout << t << endl;
	if (env.tokens.size () < 2) {
		std::stringstream msg;
		msg << "malformed word " << t;
		throw std::runtime_error (msg.str ());
	}
	Atom* word = new Atom ();

	while (env.tokens.front () != ";") {
		word->tail.push_back(env.tokens.front ());
		env.tokens.pop_front ();
	}
	env.lookup[t] = word;
}
void print (Environment& env) {
	for (size_t i = 0; i < env.stack.size (); i++) {
		*env.out << env.stack[i] << " ";
	}
	*env.out << std::endl;
}
void dump (Environment& env) {
	for (Lookup::iterator it = env.lookup.begin (); it != env.lookup.end (); ++it) {
		*env.out << it->first << " ";
	}
	*env.out << std::endl;
}
void drop (Environment& env) { env.stack.pop_back (); }
void pick (Environment& env) {
	int pos = env.stack.back (); env.stack.pop_back ();
	std::deque<Real>::iterator it = env.stack.end() - 1;
	while (pos--) it--;
	env.stack.push_back (*it);
}
void roll (Environment& env) {
	int pos = env.stack.back (); env.stack.pop_back ();
	std::deque<Real>::iterator it = env.stack.end() - 1;
	while (pos--) it--;
	env.stack.push_back (*it);
	env.stack.erase(it);
}

// UI
void repl (Environment& env, std::istream& in) {
	while (true) {
		std::string line;
		*env.out << ">> ";
		getline (in, line);
		tokenize(line, env.tokens);
		eval (env);
		env.tokens.clear ();
	}
}
int main (int argc, char* argv[]) {
	try {
		Environment env (&std::cin, &std::cout);
		env.lookup["\\"] = new Atom (&comment);
		env.lookup[":"] = new Atom (&define);
		env.lookup["."] = new Atom (&print);
		env.lookup["dump"] = new Atom (&dump);
		env.lookup["drop"] = new Atom (&drop);
		env.lookup["pick"] = new Atom (&pick);
		env.lookup["roll"] = new Atom (&roll);

		repl (env, std::cin);
	}
	catch (exception& e) {
		cout << "Error: " << e.what () << endl;
	}
	catch (...) {
		cout << "Fatal error: unknown exception" << endl;
	}
	return 0;
}

// EOF
