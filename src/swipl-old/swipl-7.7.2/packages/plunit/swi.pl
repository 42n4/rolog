/*  Part of SWI-Prolog

    Author:        Jan Wielemaker
    E-mail:        J.Wielemaker@vu.nl
    WWW:           http://www.swi-prolog.org
    Copyright (c)  2006-2012, University of Amsterdam
                              VU University Amsterdam
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:

    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.

    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in
       the documentation and/or other materials provided with the
       distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
    FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
    COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
    BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
    LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
    ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE.
*/

:- module(swi,
          [ (=@=)/2,                    % @Term1, @Term2
            forall/2,                   % :Cond, :Action
            maplist/2,                  % :Goal, ?List
            maplist/3,                  % :Goal, ?List, ?List
            ignore/1,                   % :Goal
            call/2,                     % :Goal, +A1
            call/3,                     % :Goal, +A1, +A2
            source_location/2,          % -File, -Line
            copy_term/3,                % +Term, ?Copy
            option/2,                   % +Term, +List
            option/3,                   % +Term, +List, +Default
            atomic_list_concat/2,               % +List, -Atom
            pairs_values/2,             % +Pairs, -Values
            group_pairs_by_key/2,       % +Pairs, -Groups
            thread_self/1               % -Thread
          ]).
:- meta_predicate
    forall(:,:),
    maplist(:,?),
    maplist(:,?,?),
    ignore(:),
    call(:,+),
    call(:,+,+).

:- use_module(library(lists)).
:- use_module(library(terms)).
:- op(700, xfx, =@=).

/** <module> SWI-Predicates for SICStus

This module defines some SWI-Prolog specific   predicates to support the
PlUnit environment in SICStus. Tested and  developed with SICStus Prolog
3.12.7.

@author         Jan Wielemaker
@license        GPL+SWI-exception or Artistic 2.0
*/

%% copy_term(+Term,?Copy, Goals)

copy_term(Term, Copy, Goals) :-
    call_residue(copy_term(Term,Copy0),Residuum),
    Copy0 = Copy,
    pairs_values(Residuum, Goals).

%!  =@=(A, B)
%
%   True if A is structural equivalent to  B. This means either A ==
%   B, or B is like a copy of A.

A =@= B :-
    variant(A, B).

%!  forall(:Cond, :Action) is semidet.
%
%   True if for all solutions of Cond, Action is true

forall(Cond, Action) :-
    \+ (Cond, \+ Action).


%!  maplist(:Goal, +List)
%
%   True if Goal can succesfully be applied on all elements of List.

maplist(Goal, List) :-
    maplist2(List, Goal).

maplist2([], _).
maplist2([Elem|Tail], Goal) :-
    call(Goal, Elem),
    maplist2(Tail, Goal).

%       maplist(:Goal, ?List1, ?List2)
%
%       True if Goal can succesfully be applied to all succesive pairs
%       of elements of List1 and List2.

maplist(Goal, List1, List2) :-
    maplist2(List1, List2, Goal).

maplist2([], [], _).
maplist2([Elem1|Tail1], [Elem2|Tail2], Goal) :-
    call(Goal, Elem1, Elem2),
    maplist2(Tail1, Tail2, Goal).

%!  ignore(:Goal)
%
%   Ignore failure of Goal.

ignore(Goal) :-
    (   Goal
    ->  true
    ;   true
    ).

mgoal_to_module_goal(Module0:Goal0, Module, Goal) :-
    nonvar(Goal0),
    !,
    mgoal_(Goal0, Module0,Module, Goal).
mgoal_to_module_goal(MGoal, _, _) :-
    throw(error(existence_error(module_prefix,MGoal),!)).

mgoal_(Module0:Goal0, _, Module, Goal) :-
    nonvar(Goal0),
    !,
    mgoal_(Goal0, Module0, Module, Goal).
mgoal_(Goal, Module,Module, Goal).

%!  call(:Goal, +A1, +A2)
%
%   Call with extended arguments

call(MGoal, A1, A2) :-
    mgoal_to_module_goal(MGoal, M, Goal),
    Goal =.. List,
    append(List, [A1,A2], List2),
    Goal2 =.. List2,
    call(M:Goal2).

%!  call(:Goal, +A1)
%
%   Call with extended arguments

call(MGoal, A1) :-
    mgoal_to_module_goal(MGoal, M, Goal),
    Goal =.. List,
    append(List, [A1], List2),
    Goal2 =.. List2,
    call(M:Goal2).

%!  source_location(File, Line)
%
%   Location from where we just read the last term.

source_location(File, Line) :-
    prolog_load_context(file, File),
    prolog_load_context(term_position, Pos),
    stream_position_data(line_count, Pos, Line).


                 /*******************************
                 *            OPTIONS           *
                 *******************************/

%!  option(?Option, +OptionList, +Default)
%
%   Get  an  option  from  a  OptionList.  OptionList  can  use  the
%   Name=Value as well as the Name(Value) convention.
%
%   @param Option   Term of the form Name(?Value).

option(Opt, Options, Default) :-        % make option processing stead-fast
    compound(Opt),
    arg(1, Opt, OptVal),
    nonvar(OptVal),
    !,
    functor(Opt, OptName, 1),
    functor(Gen, OptName, 1),
    option(Gen, Options, Default),
    Opt = Gen.
option(Opt, Options, _) :-
    get_option(Opt, Options),
    !.
option(Opt, _, Default) :-
    arg(1, Opt, Default).

%!  option(?Option, +OptionList)
%
%   Get  an  option  from  a  OptionList.  OptionList  can  use  the
%   Name=Value as well as the Name(Value) convention. Fails silently
%   if the option does not appear in OptionList.
%
%   @param Option   Term of the form Name(?Value).

option(Opt, Options) :- % make option processing stead-fast
    compound(Opt),
    arg(1, Opt, OptVal),
    nonvar(OptVal),
    !,
    functor(Opt, OptName, 1),
    functor(Gen, OptName, 1),
    option(Gen, Options),
    Opt = Gen.
option(Opt, Options) :-
    get_option(Opt, Options),
    !.


get_option(Opt, Options) :-
    memberchk(Opt, Options),
    !.
get_option(Opt, Options) :-
    functor(Opt, OptName, 1),
    arg(1, Opt, OptVal),
    memberchk(OptName=OptVal, Options),
    !.

%!  atomic_list_concat(+Atoms:list(atomic), -Atom:atom) is det.
%
%   Concatenate a list of atomic items to an atom.

atomic_list_concat(List, Atom) :-
    to_codes(List, Codes, []),
    atom_codes(Atom, Codes).

to_codes([], Tail, Tail).
to_codes([H|T], List, Tail) :-
    (   number(H)
    ->  number_codes(H, CL)
    ;   atom_codes(H, CL)
    ),
    append(CL, Tail0, List),
    to_codes(T, Tail0, Tail).

%!  pairs_values(+Pairs, -Values) is det.
%
%   Remove the keys  from  a  list   of  Key-Value  pairs.  Same  as
%   pairs_keys_values(Pairs, _, Values)

pairs_values([], []).
pairs_values([_-V|T0], [V|T]) :-
    pairs_values(T0, T).

%!  group_pairs_by_key(+Pairs, -Joined:list(Key-Values)) is det.
%
%   Group values with the same key.  For example:
%
%   ==
%   ?- group_pairs_by_key([a-2, a-1, b-4], X).
%
%   X = [a-[2,1], b-[4]]
%   ==
%
%   @param  Pairs   Key-Value list, sorted to the standard order
%                   of terms (as keysort/2 does)
%   @param  Joined  List of Key-Group, where Group is the
%                   list of Values associated with Key.

group_pairs_by_key([], []).
group_pairs_by_key([M-N|T0], [M-[N|TN]|T]) :-
    same_key(M, T0, TN, T1),
    group_pairs_by_key(T1, T).

same_key(M, [M-N|T0], [N|TN], T) :-
    !,
    same_key(M, T0, TN, T).
same_key(_, L, [], L).


%!  thread_self(-ThreadId) is det.
%
%   Unify ThreadId with the identifier of the calling thread.

thread_self(main).
