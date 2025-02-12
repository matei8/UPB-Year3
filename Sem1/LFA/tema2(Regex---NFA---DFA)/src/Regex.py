from dataclasses import dataclass
from typing import List, Union as PyUnion
from .NFA import NFA

EPSILON = ''


@dataclass
class Regex:
    def thompson(self) -> NFA[int]:
        raise NotImplementedError('thompson method must be implemented by subclasses')


@dataclass
class Epsilon(Regex):
    def thompson(self) -> NFA[int]:
        return NFA(
            S={EPSILON},
            K={0, 1},
            q0=0,
            d={(0, EPSILON): {1}},
            F={1}
        )


@dataclass
class Character(Regex):
    char: str

    def thompson(self) -> NFA[int]:
        return NFA(
            S={self.char},
            K={0, 1},
            q0=0,
            d={(0, self.char): {1}},
            F={1}
        )


@dataclass
class Concat(Regex):
    left: 'Regex'
    right: 'Regex'

    def thompson(self) -> NFA[int]:
        left_nfa = self.left.thompson()
        right_nfa = self.right.thompson()

        max_left_state = max(left_nfa.K)
        remapped_right_nfa = right_nfa.remap_states(lambda x: x + max_left_state + 1)

        combined_states = left_nfa.K.union(remapped_right_nfa.K)
        combined_alphabet = left_nfa.S.union(remapped_right_nfa.S)
        combined_transitions = left_nfa.d.copy()
        combined_transitions.update(remapped_right_nfa.d)

        for final_state in left_nfa.F:
            combined_transitions[(final_state, EPSILON)] = {max_left_state + 1}

        return NFA(
            S=combined_alphabet,
            K=combined_states,
            q0=left_nfa.q0,
            d=combined_transitions,
            F=remapped_right_nfa.F
        )


@dataclass
class Union(Regex):
    top: 'Regex'
    bottom: 'Regex'

    @classmethod
    def from_iterable(cls, iterable):
        iterator = iter(iterable)
        first = next(iterator, None)
        if first is None:
            return None

        result = first
        for item in iterator:
            result = cls(result, item)
        return result

    def thompson(self) -> NFA[int]:
        top_nfa = self.top.thompson()
        bottom_nfa = self.bottom.thompson()

        max_top_state = max(top_nfa.K)
        remapped_bottom_nfa = bottom_nfa.remap_states(lambda x: x + max_top_state + 2)

        new_initial_state = max(top_nfa.K) + max(bottom_nfa.K) + 3
        new_final_state = new_initial_state + 1

        combined_states = {new_initial_state, new_final_state}.union(top_nfa.K).union(remapped_bottom_nfa.K)
        combined_alphabet = top_nfa.S.union(remapped_bottom_nfa.S)
        combined_transitions = top_nfa.d.copy()
        combined_transitions.update(remapped_bottom_nfa.d)

        combined_transitions[(new_initial_state, EPSILON)] = {top_nfa.q0, remapped_bottom_nfa.q0}
        for top_final in top_nfa.F:
            combined_transitions[(top_final, EPSILON)] = {new_final_state}
        for bottom_final in remapped_bottom_nfa.F:
            combined_transitions[(bottom_final, EPSILON)] = {new_final_state}

        return NFA(
            S=combined_alphabet,
            K=combined_states,
            q0=new_initial_state,
            d=combined_transitions,
            F={new_final_state}
        )


@dataclass
class Star(Regex):
    regex: 'Regex'

    def thompson(self) -> NFA[int]:
        inner_nfa = self.regex.thompson()

        max_inner_state = max(inner_nfa.K)
        new_initial_state = max_inner_state + 1
        new_final_state = new_initial_state + 1

        combined_states = inner_nfa.K.union({new_initial_state, new_final_state})
        combined_transitions = inner_nfa.d.copy()

        combined_transitions[(new_initial_state, EPSILON)] = {inner_nfa.q0, new_final_state}
        for final_state in inner_nfa.F:
            combined_transitions[(final_state, EPSILON)] = {inner_nfa.q0, new_final_state}

        return NFA(
            S=inner_nfa.S,
            K=combined_states,
            q0=new_initial_state,
            d=combined_transitions,
            F={new_final_state}
        )


@dataclass
class Optional(Regex):
    regex: 'Regex'

    def thompson(self) -> NFA[int]:
        inner_nfa = self.regex.thompson()

        max_inner_state = max(inner_nfa.K)
        new_initial_state = max_inner_state + 1
        new_final_state = new_initial_state + 1

        combined_states = inner_nfa.K.union({new_initial_state, new_final_state})
        combined_transitions = inner_nfa.d.copy()

        combined_transitions[(new_initial_state, EPSILON)] = {inner_nfa.q0, new_final_state}
        for final_state in inner_nfa.F:
            combined_transitions[(final_state, EPSILON)] = {new_final_state}

        return NFA(
            S=inner_nfa.S,
            K=combined_states,
            q0=new_initial_state,
            d=combined_transitions,
            F={new_final_state}
        )


@dataclass
class Plus(Regex):
    regex: 'Regex'

    def thompson(self) -> NFA[int]:
        inner_nfa = self.regex.thompson()

        max_inner_state = max(inner_nfa.K)
        new_initial_state = max_inner_state + 1
        new_final_state = new_initial_state + 1

        combined_states = inner_nfa.K.union({new_initial_state, new_final_state})
        combined_transitions = inner_nfa.d.copy()

        combined_transitions[(new_initial_state, EPSILON)] = {inner_nfa.q0}
        for final_state in inner_nfa.F:
            combined_transitions[(final_state, EPSILON)] = {inner_nfa.q0, new_final_state}

        return NFA(
            S=inner_nfa.S,
            K=combined_states,
            q0=new_initial_state,
            d=combined_transitions,
            F={new_final_state}
        )


def is_special_char(char: str) -> bool:
    return char in ['*', '+', '?', '|', '(', ')', '\\']


def parse_regex(regex: str) -> Regex:
    def parse(tokens: List[str]) -> Regex | None:
        result = parse_union(tokens)
        if result is None or tokens:
            return None
        return result

    def parse_union(tokens):
        left = parse_concat(tokens)
        while tokens and tokens[0] == '|':
            tokens.pop(0)  # Consume '|'
            right = parse_concat(tokens)
            left = Union(left, right)  # Combine using a Union node
        return left

    def parse_concat(tokens: List[str]) -> None | Concat | Regex:
        left = parse_star(tokens)
        if left is None:
            return None

        while True:
            lookahead_tokens = tokens.copy()
            right = parse_star(lookahead_tokens)

            if right is None:
                break

            tokens[:] = lookahead_tokens
            left = Concat(left, right)

        return left

    def parse_star(tokens: List[str]) -> None | Optional | Plus | Star | Regex:
        base = parse_base(tokens)
        if base is None:
            return None

        while tokens and tokens[0] in ['*', '+', '?']:
            op = tokens.pop(0)
            if op == '*':
                base = Star(base)
            elif op == '+':
                base = Plus(base)
            elif op == '?':
                base = Optional(base)

        return base

    def parse_base(tokens: List[str]) -> None | Regex | Epsilon | Character | List[str]:
        if not tokens:
            return None

        token = tokens[0]

        # Parenthesized expressions
        if token == '(':
            tokens.pop(0)

            inner_tokens = []
            paren_depth = 1

            while tokens and paren_depth > 0:
                current = tokens[0]
                if current == '(':
                    paren_depth += 1
                elif current == ')':
                    paren_depth -= 1

                if paren_depth > 0:
                    inner_tokens.append(current)
                    tokens.pop(0)

            if paren_depth != 0:
                return None

            if tokens and tokens[0] == ')':
                tokens.pop(0)

            return parse(inner_tokens)

        # Epsilon
        if token == 'eps':
            tokens.pop(0)
            return Epsilon()

        # Expanded character class handling for ranges
        if token.startswith('[') and token.endswith(']'):
            tokens.pop(0)
            char_class = token[1:-1]

            if len(char_class) >= 3 and char_class[1] == '-':
                start, end = char_class[0], char_class[2]
                range_chars = [Character(chr(c)) for c in range(ord(start), ord(end) + 1)]
                return Union.from_iterable(range_chars)

            range_chars = []
            i = 0
            while i < len(char_class):
                if i + 2 < len(char_class) and char_class[i + 1] == '-':
                    start, end = char_class[i], char_class[i + 2]
                    range_chars.extend([Character(chr(c)) for c in range(ord(start), ord(end) + 1)])
                    i += 3
                else:
                    range_chars.append(Character(char_class[i]))
                    i += 1

            return Union.from_iterable(range_chars)

        # Single characters
        if tokens[0] not in ['*', '+', '?', '|', '(', ')', '\\']:
            token = tokens.pop(0)
            return Character(token)
        elif token == '\\':
            tokens.pop(0)
            token = tokens.pop(0)
            return Character(token)

        if is_special_char(token):
            return None

    # Tokenization
    def tokenize(regex: str) -> List[str]:
        cleaned_regex = ''.join(
            char for i, char in enumerate(regex)
            if char != ' ' or (i > 0 and regex[i - 1] == '\\')
        ).replace('\\ ', ' ')

        tokens = []
        i = 0
        while i < len(cleaned_regex):
            char = cleaned_regex[i]

            if char == '[':
                j = i + 1
                while j < len(cleaned_regex) and cleaned_regex[j] != ']':
                    j += 1
                if j == len(cleaned_regex):
                    raise ValueError("Unmatched '[' in regex")
                tokens.append(cleaned_regex[i:j + 1])
                i = j + 1
                continue

            if is_special_char(char):
                tokens.append(char)
            else:
                tokens.append(char)

            i += 1

        return tokens

    tokens = tokenize(regex)
    result = parse(tokens)
    if result is None:
        raise ValueError(f"Invalid regex: {regex}")
    return result
