# DATANORM Parser
Datanorm -> CSV (; separated and stored as txt)

Allowed set order:
 + R -> T -> A -> B -> P
 
Specifications via DATANORM:
 + R set is located in a separate file (.RAB)
 + If B entry is present for an article, the B entry is directly after the A entry
 + P set is located in a separate file (DATPREIS)

## Execution
`gcc datanormparser.c -o dnp`

`./dnp [filename1] [filename2] ...`

### Example
`gcc datanormparser.c -o dnp`

`./dnp datanorm.001.html datanorm.006.html datpreis.006.html`

## ToDos:
 + Write out all fields (incl. discount types)
 + Article groups
 + Sleep 😴
 
 ## FAQ
 ### Why C?
 Idk, perfomance oder so
