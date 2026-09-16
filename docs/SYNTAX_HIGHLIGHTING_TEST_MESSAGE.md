# Syntax Highlighting Validation Message

Use the following blocks when validating the code-language selector and syntax presentation on the legacy target.

## Python

````text
```python
# Pentium 4 syntax test
count = 4

def greet(name):
    if name is None:
        return "Hello :D"
    return "Hello, " + name

print(greet("Server 2003"))
```
````

## C++

````text
```cpp
#include <stdio.h>

// The emoticon aliases must remain literal inside code.
int main() {
    const char* face = ":D";
    const int count = 4;

    if (count > 0) {
        printf("Pentium 4 <3 %s\n", face);
    }

    return 0;
}
```
````

## JSON

````text
```json
{
    "machine": "Pentium 4",
    "alive": true,
    "retries": 3,
    "error": null
}
```
````

## HTML

````text
```html
<!-- Keep :D literal here. -->
<div class="legacy-machine">
    <strong>Pentium 4</strong>
</div>
```
````

After each message, verify that the code block header reports the selected language, the Copy button returns only literal source text, and aliases such as `:D`/`<3` remain literal inside code while ordinary prose aliases remain graphical.
