# CaDiCAll

[Thesis](/tex/thesis.pdf) | [CaDiCAll](/CaDiCAll/) | [Slides](/presentation/slides.pdf)

The Boolean satisfiability problem (SAT) is a fundamental NP-complete problem with nu-
merous applications in verification and reasoning. While modern Conflict-Driven Clause
Learning (CDCL) based SAT solvers efficiently compute single satisfying assignments,
many applications require enumerating all solutions (AllSAT). This is challenging due to
their potentially exponential number and the inefficiency of standard techniques such as
blocking clauses.
Implementing a model enumerator through the IPASIR-UP interface allows to lever-
age highly optimized SAT solvers such as CaDiCaL while maintaining a clear separation
between the solver and the enumeration logic. This modular approach ensures that the
enumerator remains independent of a specific solver implementation, requiring only com-
pliance with the interface. As a result, improvements in underlying solvers can be directly
utilized without modifying the enumerator, enabling both flexibility and long-term main-
tainability.
This thesis presents CaDiCAll: a model enumerator without blocking clauses using
the IPASIR-UP interface.
