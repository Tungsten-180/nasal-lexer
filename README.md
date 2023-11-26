# __INTERNAL LS NASAL INTERPRETER__
ABSOLUTELY NO WARRANTY
YOU ARE FREE TO REDISTRIBUTE AND MODIFY THE PROGRAM SO LONG AS YOU ABIDE BY THE TERMS IN THE LICENSE
(see LICENSE)
<br>
####How to use
See Nasal-ls for intended use case
This is not intended to be used alone though you may find it useful. Because of this I have made the command line logic very simple. It simply looks for whether or not you pass 'n' as the second arg(a space will count as the first). If so it will read the first line of input on stdin as the name of the file.
<br>
####Why I modified the Interpreter
 I was horified by the idea of working without an lsp for Nasal(Flightgear Scripting language)
 <br>
 (Not really but it was annoying and this was a good project for me)
<br>
Instead of doubling the work by making an AST only to dump it as text, only to re parse it back into some other form, the interpreter has been modified to output to stdout upon variable declarations, or any use, as well as function declarations and function calls. More features may be added in the future.
 <br>
 It is now primarily a level and parser, though all execution code is retained so in the future I may utilize some of the ability to execute code to improve the LSP(? I'm not sure just theorizing).
## Many thanks to ValKmjolnir 

I'm new to this so any suggestions, contributions, etc. are welcome :)
