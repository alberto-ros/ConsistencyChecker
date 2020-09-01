/** 
 *  @file    consistency.cpp
 *  @author  Alberto Ros (aros@ditec.um.es)
 *  
 *  @section DESCRIPTION
 *  
 *  A tool for obtaining all possible outcomes of a program
 *  under two consistency models IBM370 and x86-TSO.
 *
 *  Used in the paper: Alberto Ros and Stefanos Kaxiras, 
 *  "Speculative Enforcement of Store Atomicity", MICRO 2020. 
 *
 */

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <set>
#include <assert.h>

using namespace std;

#define MAX_THREADS 6
#define MAX_INSTRUCTIONS 10

typedef struct instr_ {
  bool store;
  string mem;
  int value;
} Instr;

Instr program[MAX_THREADS][MAX_INSTRUCTIONS];
int num_threads = 0;
int num_instrs[MAX_THREADS];
int max_instrs = 0;


// Program order
bool po[MAX_THREADS][MAX_INSTRUCTIONS][MAX_INSTRUCTIONS];

void add_po_ibm(int t, int i) {
  for (int d = 0; d < i+1; d++) {
    po[t][d][i] = false;
  }
  for (int d = i+1; d < num_instrs[t]; d++) {
    po[t][d][i] = false;
    if (program[t][i].store) {
      if (program[t][d].store) {
	po[t][d][i] = true;
      } else if (program[t][i].mem == program[t][d].mem) {
	po[t][d][i] = true;
      }
    } else { // load
      po[t][d][i] = true;
    }
  }
}

void add_po_tso(int t, int i) {
  for (int d = 0; d < i+1; d++) {
    po[t][d][i] = false;
  }
  for (int d = i+1; d < num_instrs[t]; d++) {
    po[t][d][i] = false;
    if (program[t][i].store) {
      if (program[t][d].store) {
	po[t][d][i] = true;
      }
    } else { // load
      po[t][d][i] = true;
    }
  }
}

void remove_po(int t, int i) {
  for (int d = i+1; d < num_instrs[t]; d++) {
    po[t][d][i] = false;
  }
}

// Build program order graph
void build_po_graph_ibm() {
  for (int t = 0; t < num_threads; t++) {
    for (int i = 0; i < num_instrs[t]; i++) {
      add_po_ibm(t, i);
    }
  }
}

void build_po_graph_tso() {
  for (int t = 0; t < num_threads; t++) {
    for (int i = 0; i < num_instrs[t]; i++) {
      add_po_tso(t, i);
    }
  }
}

bool has_po_dependencies(int thread, int instr) {
  for (int d = 0; d < num_instrs[thread]; d++) {
    if (po[thread][instr][d]) return true;
  }
  return false;
}


// Executed
bool executed[MAX_THREADS][MAX_INSTRUCTIONS];

void reset_executed() {
  for (int t = 0; t < num_threads; t++) {
    for (int i = 0; i < num_instrs[t]; i++) {
      executed[t][i] = false;
    }
  }
}

bool all_executed() {
  for (int t = 0; t < num_threads; t++) {
    for (int i = 0; i < num_instrs[t]; i++) {
      if (!executed[t][i]) return false;
    }
  }
  return true;
}


// Memory and loaded values
string memvars[MAX_THREADS*MAX_INSTRUCTIONS];
int memvalues[MAX_THREADS*MAX_INSTRUCTIONS];
int num_memvars = 0;
int loadvalues[MAX_THREADS][MAX_INSTRUCTIONS];

void insert_memvar(string var) {
  for (int pos = 0; pos < num_memvars; pos++) {
    if (memvars[pos].compare(var) == 0) {
      return;
    }
  }
  memvars[num_memvars] = var;
  memvalues[num_memvars] = 0;
  num_memvars++;
}

int get_memvar(string var) {
  for (int pos = 0; pos < num_memvars; pos++) {
    if (memvars[pos].compare(var) == 0) {
      return memvalues[pos];
    }
  }
  assert(false);
}

int update_memvar(string var, int value) {
  for (int pos = 0; pos < num_memvars; pos++) {
    if (memvars[pos].compare(var) == 0) {
      int aux = memvalues[pos];
      memvalues[pos] = value;
      return aux;
    }
  }
  assert(false);
}

int update_load(int thread, int instr, int value) {
  int aux = loadvalues[thread][instr];
  loadvalues[thread][instr] = value;
  return aux;
}

void print_mem(ostream& out) {
  for (int pos = 0; pos < num_memvars; pos++) {
    out << "[" << memvars[pos] << "]==" << memvalues[pos] << "; ";
  }
}

void print_loads(ostream& out) {
  for (int t = 0; t < num_threads; t++) {
    for (int i = 0; i < num_instrs[t]; i++) {
      if (!program[t][i].store) {
	//out << program[t][i].mem << "_" << t+1 << "_" << i+1 << "==" << loadvalues[t][i] << "; ";
     	out << program[t][i].mem << "==" << loadvalues[t][i] << "; ";
      }
    }
  }
}

// Solutions
set<string> solutions_ibm;
set<string> solutions_tso;

void add_possible_execution_ibm() {
  stringstream ss;
  print_mem(ss);
  print_loads(ss);
  solutions_ibm.insert(ss.str());
}

void add_possible_execution_tso() {
  stringstream ss;
  print_mem(ss);
  print_loads(ss);
  solutions_tso.insert(ss.str());
}

// Given a program order, get all possible executions
void get_possible_executions_ibm() {
  for (int t = 0; t < num_threads; t++) {
    for (int i = 0; i < num_instrs[t]; i++) {
      if (!executed[t][i] && !has_po_dependencies(t, i)) {
	
	// execute instruction
	executed[t][i] = true;
	int old_val;
	if (program[t][i].store) {
	  old_val = update_memvar(program[t][i].mem, program[t][i].value);
	} else {
	  old_val = update_load(t, i, get_memvar(program[t][i].mem)); // IBM370
	}

	// remove dependencies
	remove_po(t, i);

	// Check end or recursive
	if (all_executed()) {
	  add_possible_execution_ibm();
	} else {
	  get_possible_executions_ibm();
	}
	
	// reverse
	add_po_ibm(t, i);

	if (program[t][i].store) {
	  update_memvar(program[t][i].mem, old_val);
	} else {
	  update_load(t, i, 0);
	}
	executed[t][i] = false;
      }
    }
  }
}

bool is_prevstore(int thread, int instr) {
  assert(!program[thread][instr].store);
  for (int i = instr - 1; i >= 0; i--) {
    if (program[thread][i].store
	&& program[thread][i].mem.compare(program[thread][instr].mem) == 0) {
      return true;
    }
  }
  return false;
}

bool is_prevstore_executed(int thread, int instr) {
  assert(!program[thread][instr].store);
  for (int i = instr - 1; i >= 0; i--) {
    if (program[thread][i].store
	&& program[thread][i].mem.compare(program[thread][instr].mem) == 0) {
      if (executed[thread][i]) {
	return true;
      }
    }
  }
  return false;
}

int get_prevstore(int thread, int instr) {
  assert(!program[thread][instr].store);
  for (int i = instr - 1; i >= 0; i--) {
    if (program[thread][i].store
	&& program[thread][i].mem.compare(program[thread][instr].mem) == 0) {
      return program[thread][i].value;
    }
  }
  assert(false);
}

// Given a program order, get all possible executions
void get_possible_executions_tso() {
  for (int t = 0; t < num_threads; t++) {
    for (int i = 0; i < num_instrs[t]; i++) {
      if (!executed[t][i] && !has_po_dependencies(t, i)) {
	
	// execute instruction
	executed[t][i] = true;

	// remove dependencies
	remove_po(t, i);

	int old_val = get_memvar(program[t][i].mem);
	if (program[t][i].store) {
	  update_memvar(program[t][i].mem, program[t][i].value);
	} else {
	  if (is_prevstore(t, i) && !is_prevstore_executed(t, i)) {
	    update_load(t, i, get_prevstore(t, i)); // TSO
	  } else {
	    update_load(t, i, get_memvar(program[t][i].mem)); // IBM370
	  }
	}

	// Check end or recursive
	if (all_executed()) {
	  add_possible_execution_tso();
	} else {
	  get_possible_executions_tso();
	}

	if (!program[t][i].store && is_prevstore(t, i) && !is_prevstore_executed(t, i)) {
	  update_load(t, i, get_prevstore(t, i)); // TSO
	  // Check end or recursive
	  if (all_executed()) {
	    add_possible_execution_tso();
	  } else {
	    get_possible_executions_tso();
	  }
	}

	// reverse
	if (program[t][i].store) {
	  update_memvar(program[t][i].mem, old_val);
	} else {
	  update_load(t, i, 0);
	}
	add_po_tso(t, i);
	executed[t][i] = false;
      }
    }
  }
}

void print_instr(Instr i) {
  if (i.store) {
    cout << "st " << i.mem << ", " << i.value;   
  } else {
    cout << "ld " << i.mem;
  }
}
  
void print_program() {
  for (int i = 0; i < max_instrs; i++) {
    for (int t = 0; t < num_threads; t++) {
      if (i < num_instrs[t]) {
	print_instr(program[t][i]);
	cout << "\t\t";
      } else {
	cout << "\t\t\t";
      }
    }
    cout << endl;
  }
}

int main (int argc, char *argv[]) {  
  string line;
  int num_thread = 0;
  int num_instr = 0;
  do {
    getline(cin, line);
    stringstream ss;
    ss.str(line);
    //cout << line << endl;
    Instr i;
    string op;
    ss >> op;
    if (op.compare("---") == 0) {
      num_instrs[num_thread] = num_instr;
      if (num_instr > max_instrs) {
	max_instrs = num_instr;
      }
      num_thread++;
      assert(num_thread < MAX_THREADS);
      num_instr = 0;
    } else if (op.compare("st") == 0) {
      i.store = true;
      ss >> i.mem;
      ss >> i.value;
      //cout << num_thread << " " << num_instr << " "; print_instr(i); cout << endl;
      program[num_thread][num_instr] = i;
      num_instr++;
      insert_memvar(i.mem);
    } else if (op.compare("ld") == 0) { 
      i.store = false;
      ss >> i.mem;
      //cout << num_thread << " " << num_instr << " "; print_instr(i); cout << endl;
      program[num_thread][num_instr] = i;
      num_instr++;
      assert(num_instr < MAX_INSTRUCTIONS);
      insert_memvar(i.mem);
      loadvalues[num_thread][num_instr] = 0;
    } else if (op.compare("") == 0) { 
    } else {
      cout << op << endl;
      assert(false);
    }
  } while(!cin.eof());
  num_threads = num_thread;
  cout << "PROGRAM LOADED:" << endl;
  print_program();
  cout << endl;
  
  reset_executed();  
  solutions_ibm.clear();
  build_po_graph_ibm();
  get_possible_executions_ibm();
  cout << "IBM370 (STORE-ATOMIC) POSSIBLE SOLUTIONS:" << endl;
  for (auto it = solutions_ibm.begin(); it != solutions_ibm.end(); it++) {
    cout << *it << endl;
  }
  cout << endl;
  
  reset_executed();  
  solutions_tso.clear();
  build_po_graph_tso();
  get_possible_executions_tso();
  cout << "TSO (WRITE-ATOMIC) POSSIBLE SOLUTIONS (* breaks store atomicity):" << endl;
  for (auto it = solutions_tso.begin(); it != solutions_tso.end(); it++) {
    cout << *it;
    bool found = false;
    for (auto it2 = solutions_ibm.begin(); it2 != solutions_ibm.end(); it2++) {
      if ((*it).compare(*it2) == 0) {
	found = true;
      }
    }
    if (!found) {
      cout << "*";
    }
    cout << endl;
  }
  cout << endl;
  
  return 0;
}
