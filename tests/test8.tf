print

# --- costruzione numeri ---
1 2 + print            # 3
3 4 + print            # 7
dup print              # duplica 7 → 7 7

# --- confronto semplice ---
5 > print              # 5 > 7  → false
dup print              # duplica false

# --- if semplice ---
[ 100 ] if print       # se true → push 100

# --- confronto false ---
0 1 == print           # false

# --- ifelse semplice ---
[ 200 ] [ 300 ] ifelse print   # false → 300

# --- uso di call esplicito ---
[ 10 20 + ] call print         # 30

# --- confronto complesso ---
5 3 > print            # 3 > 5 → false
dup print

# --- ifelse annidato ---
[
    [ 1 2 + ]          # 3
    [ 10 20 + ]        # 30
    ifelse
] call print

# --- altro nesting ---
1 print
[
    dup
    [ 5 > ] call
    [ 100 ]
    [ 200 ]
    ifelse
] call print

# --- test combinato ---
2 3 + 4 > print        # (3+2)=5 → 4>5 → false

[
    [ 1 1 + ]          # 2
    [ 2 2 + ]          # 4
    ifelse
] call print

# --- fine ---
print
