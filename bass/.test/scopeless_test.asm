
macro assert(exp) {
  if (!({exp})) {
    error "Assert error: {exp}"
  }
}

// traditional behaviour
define count(0)
macro oldBehaviour(by) {
  global evaluate count({count} + {by})
}

oldBehaviour(2)
scope s {
	oldBehaviour(1)
	oldBehaviour(3)
}
oldBehaviour(1)

assert({count} == 3)
assert({s.count} == 6)


// new scopeless defines

define ::count(0)
macro scopeless(by) {
  global evaluate ::count({::count} + {by})
}

scopeless(2)
scope s {
	scopeless(1)
	scopeless(3)
}
scopeless(1)

assert({::count} == 7)


// Test scopeless constants

constant ::constantValue(9)

// Test evaluations work with scopeless constants
dw ::constantValue * 9

