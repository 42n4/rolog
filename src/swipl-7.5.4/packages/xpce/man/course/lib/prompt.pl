/*  Part of XPCE

    This example code is in the public domain
*/

:- module(pce_prompt,
          [ prompt/2
          ]).

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
This module defines a standard prompter-box for PCE applications.  It is
invoked with:

    prompt(+Tile, +ListOfAttributes)

where each attribute is a term of the form

   +Label:+Type = -Value[/+Default]

Examples:

prompt('Create class',
       [ name:name = Name
       , super:name = Super
       ]).

NOTE:   Package is under development.  Needs support for more types;
        optional/obligatory fields and better error-messages.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

:- pce_global(@prompter, new(dialog)).
:- pce_global(@prompter_execute_message,
              new(message(@receiver?window?ok_member, execute))).

prompt(Title, Attributes) :-
    send(@prompter, clear),
    maplist(append_prompter(@prompter), Attributes),
    send(@prompter, append,
         button(ok, message(@prompter, return, ok)), next_row),
    send(@prompter, append,
         button(cancel, message(@prompter, return, cancel))),
    send(@prompter?frame, label, Title),
    send(@prompter, fit),
    repeat,
        get(@prompter, confirm_centered, ?(@event, position, @display), OK),
        (   OK == ok
        ->  maplist(read_prompter(@prompter), Attributes),
            !,
            send(@prompter, show, @off)
        ;   !,
            send(@prompter, show, @off),
            fail
        ).


                /********************************
                *      CREATE DIALOG ITEMS      *
                ********************************/

append_prompter(P, Label:Type = Value) :-
    make_dialog_item(DI, Label, Type),
    set_default(Value, DI),
    send(P, append, DI).

                                                  % TBD: specialised types
make_dialog_item(DI, Label, _) :-
    !,
    new(DI, text_item(Label, '', @prompter_execute_message)).


                /********************************
                *          SET DEFAULTS         *
                ********************************/

set_default(Value, DI) :-
    nonvar(Value),
    Value = _RVal/Default,
    !,
    send(DI, selection, Default).
set_default(_, _).

                /********************************
                *           READ VALUES         *
                ********************************/

read_prompter(P, Label:Type = Value) :-
    get(P, member, Label, DI),
    get(DI, selection, V0),
    canonicalise(DI, V0, V1),
    (   get(@pce, convert, V1, Type, Val)
    ->  (   nonvar(Value),
            Value = RVal/_
        ->  RVal = Val
        ;   Value = Val
        )
    ;   send(@display, inform, '%s should be a %s', Label, Type),
        fail
    ).


canonicalise(DI, A, B) :-
    send(DI?class, is_a, text_item),
    !,
    new(S, string(A)),
    send(S, strip),
    get(S, value, B),
    send(S, done).
canonicalise(_, Val, Val).                                % TBD
