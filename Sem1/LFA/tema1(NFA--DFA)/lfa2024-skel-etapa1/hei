from collections.abc import Callable
from dataclasses import dataclass
from typing import TypeVar

STATE = TypeVar('STATE')

@dataclass
class DFA[STATE]:
    S: set[str]
    K: set[STATE]
    q0: STATE
    d: dict[tuple[STATE, str], STATE]
    F: set[STATE]
    

    def accept(self, word: str) -> bool:
        # simulate the dfa on the given word. return true if the dfa accepts the word, false otherwise
        current_state = self.q0 # start at the initial state

        # Parse the characters in the input word
        for char in word:
            # Transition to the next state based on the transition dictionary
            if (current_state, char) in self.d:
                current_state = self.d[(current_state, char)]
            else:
                # If no transition exists for the current state and character, reject the word
                return False

        # Check if the final state is in the set of accepting states
        if current_state in self.F:
            return True

        return False


    def remap_states[OTHER_STATE](self, f: Callable[[STATE], 'OTHER_STATE']) -> 'DFA[OTHER_STATE]':
        # optional, but might be useful for subset construction and the lexer to avoid state name conflicts.
        # this method generates a new dfa, with renamed state labels, while keeping the overall structure of the
        # automaton.

        # for example, given this dfa:

        # > (0) -a,b-> (1) ----a----> ((2))
        #               \-b-> (3) <-a,b-/
        #                   /     ⬉
        #                   \-a,b-/

        # applying the x -> x+2 function would create the following dfa:

        # > (2) -a,b-> (3) ----a----> ((4))
        #               \-b-> (5) <-a,b-/
        #                   /     ⬉
        #                   \-a,b-/

        pass


    def minimize(self) -> 'DFA[STATE]':
        # P = set of all partitions of states (accepting vs non-accepting states)
        # W = working set containing partitions that need more splitting
        P = {frozenset(self.F), frozenset(self.K - self.F)} - {frozenset()}  # Remove empty partitions
        W = P.copy()

        while W:
            A = W.pop()
            for c in self.S:  # Go through each symbol in the alphabet
                X = {} # X = set of states that have transitions on char c to any state in A
                for state in self.K:
                    if (state, c) in self.d and self.d[(state, c)] in A:
                        X[state] = self.d[(state, c)]

                X = frozenset(X.keys())
                new_P = set()  # Create a new set of partitions

                for Y in P:
                    intersection = frozenset(X & Y)  # States in Y that can transition to A on c
                    difference = frozenset(Y - X)  # States in Y that cannot transition to A on c

                    if intersection and difference:
                        # Split partition Y into intersection and difference
                        new_P.add(intersection)
                        new_P.add(difference)

                        # Update W to process new partitions
                        if Y in W:
                            W.remove(Y)
                            W.update({intersection, difference})
                        elif len(intersection) <= len(difference):
                            W.add(intersection)
                        else:
                            W.add(difference)
                    else:
                        # No split, keep Y as it is
                        new_P.add(Y)

                # Update P with the new partitions
                P = new_P

        new_states = {frozenset(partition) for partition in P}  # Each partition becomes a new DFA state
        new_initial_state = next(state for state in new_states if self.q0 in state)  # Find the new initial state
        new_accepting_states = {state for state in new_states if state & self.F}  # New accepting states
        new_transitions = {}  # New transition function

        # Build the transition dictionary for the new DFA
        for state in new_states:
            for c in self.S:
                possible_targets = {self.d[(s, c)] for s in state if (s, c) in self.d}
                if possible_targets:
                    target_state = next(new_state for new_state in new_states if possible_targets & new_state)
                    new_transitions[(state, c)] = target_state

        # Return the minimized DFA
        return DFA(
            S=self.S,  # Alphabet remains the same
            K=new_states,  # Set of new states (partitions)
            q0=new_initial_state,  # New initial state
            d=new_transitions,  # New transition function
            F=new_accepting_states  # New set of accepting states
        )
