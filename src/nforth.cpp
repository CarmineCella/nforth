// nforth.cpp
//

#include <stdexcept>
#include <iostream>
#include <deque>
#include <string>
#include <sstream>
#include <map>
#include <stdexcept>
#include <cstdlib>
#include <fstream>

using namespace std;

// types
typedef double Real;
typedef std::deque<Real> Stack;
struct Environment;
typedef void (*funcptr)(Environment&);
enum AtomType {
	NUMBER,
	LEXEME,
	FUNCTOR,
	WORD
};
struct Atom {
	Atom () { type = WORD; }
	Atom (funcptr a) { action = a; type = FUNCTOR; }
	Atom (Real v) { val = v; type = NUMBER; }
	Atom (const std::string& s) { lexeme = s; type = LEXEME; }
	AtomType type;
	Real val;
	std::string lexeme;
	funcptr action;
	std::deque<Atom*> tail;
};
typedef std::map<Atom*, Atom*> Lookup;
struct Environment {
	Environment (std::istream* input, std::ostream* output) {
		in = input;	out = output;
	}
	virtual ~Environment () {
		for (Lookup::iterator it = lookup.begin (); it != lookup.end (); ++it) {
			delete it->second;
		}
	}
	static Atom* make_symbol (const std::string& s) {
		static std::map<std::string, Atom*> sym_tab;
		std::map<std::string, Atom*>::iterator it = sym_tab.find (s);
		if (it != sym_tab.end ()) {
			return it->second;
		} else {
			Atom* a = new Atom ();
			a->lexeme = s;
			sym_tab[s] = a;
			return a;
		}
	}
	Lookup lookup;
	Stack stack;
	std::deque<Atom*> ipstack;
	std::deque<std::string> strings;
	std::istream* in;
	std::ostream* out;
};

const Atom* semicolon = Environment::make_symbol(";");
const Atom* rparen = Environment::make_symbol(")");

// parsing
bool is_number (const char* s) {
	std::istringstream iss (s);
	Real dummy;
	iss >> std::noskipws >> dummy;
	return iss && iss.eof ();
}
bool is_string (const std::string& s) {
	if (s.size () < 2) return false;
	return s[s.size () - 1] == '\"';
}
void tokenize (std::string line, std::deque<std::string>& tokens) {
	std::stringstream vals;
	vals << line;
	std::stringstream accum;
	while (!vals.eof ()) {
		char c = vals.get ();
		switch (c) {
			case '\"': {
				c = 0;
				std::stringstream strv;
				while (!vals.eof () && c != '\"') {
					c = vals.get ();
					accum << c;
				}
			}
			break;
			case ' ':
			case '\n':
			case '\r':
			case '\t':
				if (accum.str ().size ()) {
					tokens.push_back (accum.str ());
				}
				accum.str ("");
			break;
			default:
			if (c > 0) accum << c;
		}
	}
	if (accum.str ().size ()) tokens.push_back (accum.str ());
}

// functors
void eval (Environment& env) {
tail_call:
	while (env.ipstack.size ()) {
		Atom* t = env.ipstack.front (); env.ipstack.pop_front ();
		if (t->type == NUMBER) {
			env.stack.push_back (t->val);
		} else {
			Lookup::iterator it = env.lookup.find (t);
			if (it == env.lookup.end ()) {
				std::stringstream msg;
				msg << "invalid identifier " << t->lexeme;
				throw std::runtime_error (msg.str ());
			} else {
				if (it->second->type == FUNCTOR) it->second->action (env);
				else {
					for (size_t j = 0; j < env.ipstack.size (); j++) {
						it->second->tail.push_back (env.ipstack[j]);
					}
					env.ipstack = it->second->tail;
					goto tail_call;
				}
			}
		}
	}
}
void readline (Environment& env) {
	std::string line;
	getline (*env.in, line);
	env.strings.push_back (line);
}
void parse (Environment& env) {
	std::string line = env.strings.back (); env.strings.pop_back ();
	std::deque<std::string> tokens;
	tokenize (line, tokens);
	for (size_t i = 0; i < tokens.size (); i++) {
		std::string t = tokens[i];
		if (is_number(t.c_str ())) {
			env.ipstack.push_back (new Atom (atof (t.c_str ())));
		} else if (is_string (t)){
			env.strings.push_back(t.substr (0, t.size () -1));
		} else {
			env.ipstack.push_back (Environment::make_symbol(t));
		}
	}
}
void comment (Environment& env) {
	while (env.ipstack.front () != rparen) {
		env.ipstack.pop_front ();
	}
	env.ipstack.pop_front ();
}
void define (Environment& env) {
	Atom* t = env.ipstack.front (); env.ipstack.pop_front (); // remove W
	if (env.ipstack.size () < 2) {
		std::stringstream msg;
		msg << "malformed word " << t;
		throw std::runtime_error (msg.str ());
	}
	Atom* word = new Atom ();

	while (env.ipstack.front () != semicolon) {
		word->tail.push_back(env.ipstack.front ());
		env.ipstack.pop_front ();
	}
	env.ipstack.pop_front (); // remove ;
	env.lookup[t] = word;
}
void print (Environment& env) {
	for (size_t i = 0; i < env.stack.size (); i++) {
		*env.out << env.stack[i] << " ";
	}
	*env.out << std::endl;
}
void printstr (Environment& env) {
	for (size_t i = 0; i < env.strings.size (); i++) {
		*env.out << env.strings[i] << " ";
	}
	*env.out << std::endl;
}

void words (Environment& env) {
	for (Lookup::iterator it = env.lookup.begin (); it != env.lookup.end (); ++it) {
		*env.out << it->first->lexeme << " ";
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
void depth (Environment& env) {
	env.stack.push_back (env.stack.size ());
}
void while_s (Environment& env) {
	std::deque<Atom*> tail;
	while (env.ipstack.front () != semicolon) {
		tail.push_back(env.ipstack.front ());
		env.ipstack.pop_front ();
	}
	env.ipstack.pop_front (); // remove ;
	Real cond = env.stack.back (); env.stack.pop_back ();
	while (cond) {
		for (size_t i = 0; i < tail.size (); i++) {
			env.ipstack.push_back (tail[i]);
		}
		
		eval (env);
		cond = env.stack.back (); env.stack.pop_back ();
	}
}

void load (Environment& env) {
	std::string filen = env.strings.back (); env.strings.pop_back ();
	std::ifstream source (filen.c_str ());
	if (!source.good ()) {
		std::stringstream err;
		err << "cannot open " << filen;
		throw std::runtime_error (err.str ());
	}
	while (!source.eof ()) {
		std::string line;
		getline (source, line);
		cout << line << endl;
		env.strings.push_back (line);

		parse (env);
		eval (env);
	}
}

// UI
void repl (Environment& env, std::istream& in) {
	while (true) {
		std::string line;
		*env.out << ">> ";
		readline (env);
		parse (env);
		eval (env);
	}
}
int main (int argc, char* argv[]) {
	try {
		Environment env (&std::cin, &std::cout);
		env.lookup[Environment::make_symbol ("accept")] = new Atom (&readline);
		env.lookup[Environment::make_symbol ("parse")] = new Atom (&parse);
		env.lookup[Environment::make_symbol ("eval")] = new Atom (&eval);
		env.lookup[Environment::make_symbol ("(")] = new Atom (&comment);
		env.lookup[Environment::make_symbol (":")] = new Atom (&define);
		env.lookup[Environment::make_symbol (".s")] = new Atom (&print);
		env.lookup[Environment::make_symbol (".@")] = new Atom (&printstr);
		env.lookup[Environment::make_symbol ("words")] = new Atom (&words);
		env.lookup[Environment::make_symbol ("drop")] = new Atom (&drop);
		env.lookup[Environment::make_symbol ("pick")] = new Atom (&pick);
		env.lookup[Environment::make_symbol ("roll")] = new Atom (&roll);

		env.lookup[Environment::make_symbol ("load")] = new Atom (&load);
		env.lookup[Environment::make_symbol ("while")] = new Atom (&while_s);
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
