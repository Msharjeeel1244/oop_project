# Project Management System with Kanban Board

OOP semester project — **single file C++** (`main.cpp`) jo ek lightweight HTTP
server banata hai aur browser mein Kanban board serve karta hai. Frontend
(HTML/CSS/JS) C++ string ke andar embed hai, isliye sab kuch ek hi file mein hai.

## Compile

```bash
g++ -std=c++17 -pthread main.cpp -o pm
```

## Run

```bash
./pm
```

Phir browser mein kholo: **http://localhost:8080**

Band karne ke liye terminal mein `Ctrl + C`.

> Sirf standard C++ + POSIX sockets (`sys/socket.h`) use hue hain. Koi external
> library nahi. Linux/Mac dono pe chal jata hai.

## Features

| Feature | Kaise |
|---|---|
| 3 columns | To Do, In Progress, Done |
| Task add | Title + Description + Priority (Low/Medium/High) |
| Task move | Card ko **drag-drop** karo, ya card ke buttons se |
| Task delete | Card pe **Delete** button |
| Priority colors | Low = green, Medium = orange, High = red |

Data **in-memory** rakha gaya hai (server band hone pe reset). Start pe 3 demo
tasks already board pe hote hain.

## OOP Concepts (code mein kahan)

Har concept ke upar `// ===== CONCEPT =====` style comment laga hua hai.

| Concept | Class / jagah |
|---|---|
| **Abstraction** | `class Item` — pure virtual `getInfo() = 0` (abstract base class) |
| **Inheritance** | `class Task : public Item` |
| **Encapsulation** | `Task` ke private members (`title`, `description`, `priority`, `status`) + getters/setters |
| **Polymorphism** | `getInfo()` aur `getType()` virtual functions ka overriding |

## Class Structure

- **`Item`** — abstract base class (pure virtual `getInfo()`)
- **`Task : Item`** — encapsulated task (id, title, description, priority, status)
- **`Board`** — tasks ka collection manage karti hai (add / remove / move)
- **`Server`** — HTTP requests handle karti hai, HTML + JSON serve karti hai

## API Endpoints (server internal)

| Path | Method | Kaam |
|---|---|---|
| `/` | GET | Kanban board HTML page |
| `/api/tasks` | GET | Saare tasks JSON mein |
| `/api/add` | POST | Naya task add |
| `/api/move` | POST | Task ka status/column change |
| `/api/delete` | POST | Task delete |
