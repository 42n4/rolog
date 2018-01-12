/*  Part of CHR (Constraint Handling Rules)

    Author:        Tom Schrijvers
    E-mail:        Tom.Schrijvers@cs.kuleuven.be
    WWW:           http://www.swi-prolog.org
    Copyright (c)  2005-2011, K.U. Leuven
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

:- module(chr_compiler_utility,
	[ time/2
	, replicate/3
	, pair_all_with/3
	, conj2list/2
	, list2conj/2
	, disj2list/2
	, list2disj/2
	, variable_replacement/3
	, variable_replacement/4
	, identical_rules/2
	, identical_guarded_rules/2
	, copy_with_variable_replacement/3
	, my_term_copy/3
	, my_term_copy/4
	, atom_concat_list/2
	, init/2
	, member2/3
	, select2/6
	, set_elems/2
	, instrument_goal/4
	, sort_by_key/3
	, arg1/3
	, wrap_in_functor/3
	, tree_set_empty/1
	, tree_set_memberchk/2
	, tree_set_add/3
	, tree_set_merge/3
	, fold1/3
	, fold/4
	, maplist_dcg//3
	, maplist_dcg//4
	]).

:- use_module(pairlist).
:- use_module(library(lists), [permutation/2]).
:- use_module(library(assoc)).

:- meta_predicate
	fold1(3,+,-),
	fold(+,3,+,-).

%% SICStus begin
%% use_module(library(terms),[term_variables/2]).
%% SICStus end


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% time(Phase,Goal) :-
%	statistics(runtime,[T1|_]),
%	call(Goal),
%	statistics(runtime,[T2|_]),
%	T is T2 - T1,
%	format('    ~w ~46t ~D~80| ms\n',[Phase,T]),
%	deterministic(Det),
%	( Det == true ->
%		true
%	;
%		format('\t\tNOT DETERMINISTIC!\n',[])
%	).
time(_,Goal) :- call(Goal).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
replicate(N,E,L) :-
	( N =< 0 ->
		L = []
	;
		L = [E|T],
		M is N - 1,
		replicate(M,E,T)
	).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
pair_all_with([],_,[]).
pair_all_with([X|Xs],Y,[X-Y|Rest]) :-
	pair_all_with(Xs,Y,Rest).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
conj2list(Conj,L) :-				%% transform conjunctions to list
  conj2list(Conj,L,[]).

conj2list(Var,L,T) :-
	var(Var), !,
	L = [Var|T].
conj2list(true,L,L) :- !.
conj2list(Conj,L,T) :-
  Conj = (G1,G2), !,
  conj2list(G1,L,T1),
  conj2list(G2,T1,T).
conj2list(G,[G | T],T).

disj2list(Conj,L) :-				%% transform disjunctions to list
  disj2list(Conj,L,[]).
disj2list(Conj,L,T) :-
  Conj = (fail;G2), !,
  disj2list(G2,L,T).
disj2list(Conj,L,T) :-
  Conj = (G1;G2), !,
  disj2list(G1,L,T1),
  disj2list(G2,T1,T).
disj2list(G,[G | T],T).

list2conj([],true).
list2conj([G],X) :- !, X = G.
list2conj([G|Gs],C) :-
	( G == true ->				%% remove some redundant trues
		list2conj(Gs,C)
	;
		C = (G,R),
		list2conj(Gs,R)
	).

list2disj([],fail).
list2disj([G],X) :- !, X = G.
list2disj([G|Gs],C) :-
	( G == fail ->				%% remove some redundant fails
		list2disj(Gs,C)
	;
		C = (G;R),
		list2disj(Gs,R)
	).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% check wether two rules are identical

identical_guarded_rules(rule(H11,H21,G1,_),rule(H12,H22,G2,_)) :-
   G1 == G2,
   permutation(H11,P1),
   P1 == H12,
   permutation(H21,P2),
   P2 == H22.

identical_rules(rule(H11,H21,G1,B1),rule(H12,H22,G2,B2)) :-
   G1 == G2,
   identical_bodies(B1,B2),
   permutation(H11,P1),
   P1 == H12,
   permutation(H21,P2),
   P2 == H22.

identical_bodies(B1,B2) :-
   ( B1 = (X1 = Y1),
     B2 = (X2 = Y2) ->
     ( X1 == X2,
       Y1 == Y2
     ; X1 == Y2,
       X2 == Y1
     ),
     !
   ; B1 == B2
   ).

% replace variables in list

copy_with_variable_replacement(X,Y,L) :-
   ( var(X) ->
     ( lookup_eq(L,X,Y) ->
       true
     ; X = Y
     )
   ; functor(X,F,A),
     functor(Y,F,A),
     X =.. [_|XArgs],
     Y =.. [_|YArgs],
     copy_with_variable_replacement_l(XArgs,YArgs,L)
   ).

copy_with_variable_replacement_l([],[],_).
copy_with_variable_replacement_l([X|Xs],[Y|Ys],L) :-
   copy_with_variable_replacement(X,Y,L),
   copy_with_variable_replacement_l(Xs,Ys,L).

% build variable replacement list

variable_replacement(X,Y,L) :-
   variable_replacement(X,Y,[],L).

variable_replacement(X,Y,L1,L2) :-
   ( var(X) ->
     var(Y),
     ( lookup_eq(L1,X,Z) ->
       Z == Y,
       L2 = L1
     ; ( X == Y -> L2=L1 ; L2 = [X-Y,Y-X|L1])
     )
   ; X =.. [F|XArgs],
     nonvar(Y),
     Y =.. [F|YArgs],
     variable_replacement_l(XArgs,YArgs,L1,L2)
   ).

variable_replacement_l([],[],L,L).
variable_replacement_l([X|Xs],[Y|Ys],L1,L3) :-
   variable_replacement(X,Y,L1,L2),
   variable_replacement_l(Xs,Ys,L2,L3).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
my_term_copy(X,Dict,Y) :-
   my_term_copy(X,Dict,_,Y).

my_term_copy(X,Dict1,Dict2,Y) :-
   (   var(X) ->
       (   lookup_eq(Dict1,X,Y) ->
           Dict2 = Dict1
       ;   Dict2 = [X-Y|Dict1]
       )
   ;   functor(X,XF,XA),
       functor(Y,XF,XA),
       X =.. [_|XArgs],
       Y =.. [_|YArgs],
       my_term_copy_list(XArgs,Dict1,Dict2,YArgs)
   ).

my_term_copy_list([],Dict,Dict,[]).
my_term_copy_list([X|Xs],Dict1,Dict3,[Y|Ys]) :-
   my_term_copy(X,Dict1,Dict2,Y),
   my_term_copy_list(Xs,Dict2,Dict3,Ys).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
atom_concat_list([X],X) :- ! .
atom_concat_list([X|Xs],A) :-
	atom_concat_list(Xs,B),
	atomic_concat(X,B,A).

set_elems([],_).
set_elems([X|Xs],X) :-
	set_elems(Xs,X).

init([],[]).
init([_],[]) :- !.
init([X|Xs],[X|R]) :-
	init(Xs,R).

member2([X|_],[Y|_],X-Y).
member2([_|Xs],[_|Ys],P) :-
	member2(Xs,Ys,P).

select2(X, Y, [X|Xs], [Y|Ys], Xs, Ys).
select2(X, Y, [X1|Xs], [Y1|Ys], [X1|NXs], [Y1|NYs]) :-
	select2(X, Y, Xs, Ys, NXs, NYs).

instrument_goal(Goal,Pre,Post,(Pre,Goal,Post)).

sort_by_key(List,Keys,SortedList) :-
	pairup(Keys,List,Pairs),
	sort(Pairs,SortedPairs),
	once(pairup(_,SortedList,SortedPairs)).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
arg1(Term,Index,Arg) :- arg(Index,Term,Arg).

wrap_in_functor(Functor,X,Term) :-
	Term =.. [Functor,X].

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

tree_set_empty(TreeSet) :- empty_assoc(TreeSet).
tree_set_memberchk(Element,TreeSet) :- get_assoc(Element,TreeSet,_).
tree_set_add(TreeSet,Element,NTreeSet) :- put_assoc(Element,TreeSet,x,NTreeSet).
tree_set_merge(TreeSet1,TreeSet2,TreeSet3) :-
	assoc_to_list(TreeSet1,List),
	fold(List,tree_set_add_pair,TreeSet2,TreeSet3).
tree_set_add_pair(Key-Value,TreeSet,NTreeSet) :-
	put_assoc(Key,TreeSet,Value,NTreeSet).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
fold1(P,[Head|Tail],Result) :-
	fold(Tail,P,Head,Result).

fold([],_,Acc,Acc).
fold([X|Xs],P,Acc,Res) :-
	call(P,X,Acc,NAcc),
	fold(Xs,P,NAcc,Res).

maplist_dcg(P,L1,L2,L) -->
	maplist_dcg_(L1,L2,L,P).

maplist_dcg_([],[],[],_) --> [].
maplist_dcg_([X|Xs],[Y|Ys],[Z|Zs],P) -->
	call(P,X,Y,Z),
	maplist_dcg_(Xs,Ys,Zs,P).

maplist_dcg(P,L1,L2) -->
	maplist_dcg_(L1,L2,P).

maplist_dcg_([],[],_) --> [].
maplist_dcg_([X|Xs],[Y|Ys],P) -->
	call(P,X,Y),
	maplist_dcg_(Xs,Ys,P).
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
:- dynamic
	user:goal_expansion/2.
:- multifile
	user:goal_expansion/2.

user:goal_expansion(arg1(Term,Index,Arg), arg(Index,Term,Arg)).
user:goal_expansion(wrap_in_functor(Functor,In,Out), Goal) :-
	( atom(Functor), var(Out) ->
		Out =.. [Functor,In],
		Goal = true
	;
		Goal = (Out =.. [Functor,In])
	).

