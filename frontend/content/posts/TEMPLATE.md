---
title: "This is a frog!"
summary: This is a template post containing some formatting stuff for me to remember 
date: 2025-01-20
weight: 101
aliases: ["/papermod-installation"]
tags: ["This is a tag"]
author: ["Llewellyn Meldrum"]
cover:
  image: images/sandfrog.jpg
  hiddenInList: false 
social:
  fediverse_creator: "@adityatelange@mastodon.social"
---

---

# forg 

1. [forg](https://gohugo.io/getting-started/quick-start/)
2. **[Bold forg](https://gohugo.io/getting-started/quick-start/)**
3.  In text image: {{< inTextImg url="https://cdn.wcs.org/2024/03/13/21/14/05/2d2c6f1b-71c4-4390-b7e2-deb21a0bc11f/shutterstock_2331893385.jpg" height="50" >}}.
   <br>(This is a `<br>`, between the list elements, it keeps the indentation correct)

   As opposed to this, which is just below the list item 

4. # This is a buffer overwflow!
   ```c
    const int BUF_LEN = 512;
    char* buffer = malloc(sizeof(char) * BUF_LEN);
    char[513] = (char)(0xD);
   ```
   Note:
   - This is bad. 

# other stuff

This is an inline `codeblock` 

```py
this one is the ``` variant
```

**collapseable block**:
> {{< collapse summary="**expand me**" >}}
Expanded
{{</ collapse >}}

break `<br>`: 
<br>
<br>

---
> **This is a quote**: [exampleSite](https://github.com/adityatelange/hugo-PaperMod/tree/exampleSite/)
### line break/page break
 | <br>
V

---
## Quick Links

- ### [Other blog posts](../other-blog-post)

- ### [Actual link](https://google.com)

---



## end of article

---
