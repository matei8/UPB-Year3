from .DFA import DFA

from dataclasses import dataclass
from collections.abc import Callable

EPSILON = ''  # this is how epsilon is represented by the checker in the transition function of NFAs


@dataclass
class NFA[STATE]:
    S: set[str]
    K: set[STATE]
    q0: STATE
    d: dict[tuple[STATE, str], set[STATE]]
    F: set[STATE]

    def epsilon_closure(self, state: STATE) -> set[STATE]:
        # compute the epsilon closure of a state (you will need this for subset construction)
        # see the EPSILON definition at the top of this file
        result = {state}
        stack = [state]

        while stack:
            current_state = stack.pop()

            if (current_state, EPSILON) in self.d:
                for next_state in self.d[(current_state, EPSILON)]:
                    if next_state not in result:
                        result.add(next_state)
                        stack.append(next_state)

        return result

    def subset_construction(self) -> DFA[frozenset[STATE]]:
        # convert this nfa to a dfa using the subset construction algorithm
        initial_state = frozenset(self.epsilon_closure(self.q0))
        dfa_states = {initial_state}
        dfa_transitions = {}
        dfa_accepting_states = set()
        stack = [initial_state]

        while stack:
            current_states = stack.pop()

            # Check if this subset contains NFA accepting states
            if current_states & self.F:
                dfa_accepting_states.add(current_states)

            # Compute transitions for each symbol in the alphabet (excluding epsilon)
            for character in self.S - {EPSILON}:
                next_states = set()

                # Find the set of states reachable by the character
                for state in current_states:
                    if (state, character) in self.d:
                        next_states.update(self.d[(state, character)])

                # Compute the epsilon-closure of the reachable states
                closure_states = set()
                for state in next_states:
                    for closure_state in self.epsilon_closure(state):
                        closure_states.add(closure_state)

                new_dfa_state = frozenset(closure_states)

                # Add the new DFA state to the set if it hasn't been processed yet
                if new_dfa_state not in dfa_states:
                    dfa_states.add(new_dfa_state)
                    stack.append(new_dfa_state)

                # Record the transition in the DFA
                dfa_transitions[(current_states, character)] = new_dfa_state

        return DFA(
            S=self.S - {EPSILON},  # Remove epsilon from the DFA's alphabet
            K=dfa_states,  # Set of DFA states
            q0=initial_state,  # DFA initial state
            d=dfa_transitions,  # DFA transition function
            F=dfa_accepting_states  # DFA accepting states
        )


    def remap_states[OTHER_STATE](self, f: 'Callable[[STATE], OTHER_STATE]') -> 'NFA[OTHER_STATE]':
        # optional, but may be useful for the second stage of the project. Works similarly to 'remap_states'
        # from the DFA class. See the comments there for more details.

        new_states = {f(state) for state in self.K}
        new_transitions = {(f(state), char): {f(target) for target in targets} for (state, char), targets in
                           self.d.items()}
        new_initial_state = f(self.q0)
        new_accepting_states = {f(state) for state in self.F}

        return NFA(
            S=self.S,
            K=new_states,
            q0=new_initial_state,
            d=new_transitions,
            F=new_accepting_states
        )
