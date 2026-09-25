% HyperTension Logical Admissibility Authority
% made with love

:- dynamic cpp_result/2, algol_result/2, bf_membership/1, confidence_value/1.

% The threshold below which a confidence value is considered insufficient
% to permit the result to be reported as verified. This value is 95.0.
confidence_threshold(95.0).

% Both searches must agree on state AND index.
primary_agrees_with_independent_search :-
    cpp_result(present, Idx),
    algol_result(present, Idx).
primary_agrees_with_independent_search :-
    cpp_result(absent, _),
    algol_result(absent, _).

% The Bloom filter implementation used in Stage 2 guarantees no false
% negatives: every element present in the dataset will always produce
% a positive membership result. A negative result therefore conclusively
% establishes absence. A negative result combined with a found result
% from Stages 1 and 3 is physically impossible and must be rejected.
no_false_negative_contradiction :-
    \+ (cpp_result(present, _), bf_membership(negative)).

% BF positive + absent is a Bloom false positive — not a contradiction.
evidence_is_consistent :-
    primary_agrees_with_independent_search,
    no_false_negative_contradiction.

% Check confidence.
confidence_is_acceptable :-
    confidence_value(C),
    confidence_threshold(T),
    C >= T.

required_facts_present :-
    cpp_result(_, _),
    algol_result(_, _),
    bf_membership(_),
    confidence_value(_).

presence_claim_is_admissible :-
    required_facts_present,
    cpp_result(present, _),
    algol_result(present, _),
    evidence_is_consistent,
    confidence_is_acceptable.

absence_claim_is_admissible :-
    required_facts_present,
    cpp_result(absent, _),
    algol_result(absent, _),
    evidence_is_consistent,
    confidence_is_acceptable.

verified_result_is_admissible :-
    presence_claim_is_admissible.
verified_result_is_admissible :-
    absence_claim_is_admissible.

main :-
    ( verified_result_is_admissible ->
        write(admissible)
    ;
        write(inadmissible)
    ),
    % Write a newline.
    nl.
