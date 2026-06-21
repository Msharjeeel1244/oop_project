// ============================================================================
//  PROJECT MANAGEMENT SYSTEM with KANBAN BOARD
//  OOP Semester Project  -  Single File C++ + Web GUI (embedded HTML/CSS/JS)
//
//  Compile : g++ -std=c++17 -pthread main.cpp -o pm
//  Run     : ./pm
//  Open    : http://localhost:8080  (browser mein)
//
//  Sirf standard C++ + POSIX sockets use hue hain. Koi external library nahi.
// ============================================================================

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <sstream>
#include <map>
#include <mutex>
#include <algorithm>
#include <cstring>

// ---- POSIX socket headers (Linux/Mac) ----
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

// ============================================================================
//  ENUMS  -  Priority aur Status ko type-safe banane ke liye
// ============================================================================
enum class Priority { Low, Medium, High };
enum class Status   { ToDo, InProgress, Done };

// chhota helper: enum ko string mein convert karna
static std::string priorityToString(Priority p) {
    switch (p) {
        case Priority::Low:    return "Low";
        case Priority::Medium: return "Medium";
        case Priority::High:   return "High";
    }
    return "Low";
}
static std::string statusToString(Status s) {
    switch (s) {
        case Status::ToDo:       return "todo";
        case Status::InProgress: return "inprogress";
        case Status::Done:       return "done";
    }
    return "todo";
}

// ============================================================================
//  ===== ABSTRACTION =====
//  Abstract base class "Item".
//  Isme pure virtual function "getInfo()" hai -> isliye Item ka object
//  directly nahi ban sakta. Ye sirf ek "blueprint" / contract hai.
// ============================================================================
class Item {
protected:
    int id;   // derived class isko use kar sake isliye protected

public:
    Item(int id) : id(id) {}

    // pure virtual function => ABSTRACTION (must override in derived class)
    virtual std::string getInfo() const = 0;

    // ===== POLYMORPHISM ke liye virtual function =====
    // Derived class ise override kar sakti hai (default behaviour yahan).
    virtual std::string getType() const { return "Item"; }

    int getId() const { return id; }   // common getter

    // virtual destructor -> base pointer se delete safe ho (polymorphism rule)
    virtual ~Item() {}
};

// ============================================================================
//  ===== INHERITANCE =====
//  "Task" class "Item" se inherit karti hai (public inheritance).
//
//  ===== ENCAPSULATION =====
//  Saare data members private hain. Sirf getters/setters ke through access.
// ============================================================================
class Task : public Item {
private:
    // ---- ENCAPSULATED (private) data members ----
    std::string title;
    std::string description;
    Priority    priority;
    Status      status;

public:
    Task(int id, std::string title, std::string description, Priority priority)
        : Item(id),
          title(std::move(title)),
          description(std::move(description)),
          priority(priority),
          status(Status::ToDo) {}

    // ---------- GETTERS (Encapsulation) ----------
    std::string getTitle()       const { return title; }
    std::string getDescription() const { return description; }
    Priority    getPriority()    const { return priority; }
    Status      getStatus()      const { return status; }

    // ---------- SETTERS (Encapsulation) ----------
    void setTitle(const std::string& t)       { title = t; }
    void setDescription(const std::string& d) { description = d; }
    void setPriority(Priority p)              { priority = p; }
    void setStatus(Status s)                  { status = s; }

    // ===== POLYMORPHISM (function overriding) =====
    // Base class ka pure virtual getInfo() yahan override hua.
    std::string getInfo() const override {
        return "Task #" + std::to_string(id) + ": " + title +
               " [" + priorityToString(priority) + "]";
    }

    // base ka virtual getType() override -> runtime polymorphism
    std::string getType() const override { return "Task"; }

    // Task ko JSON object string mein convert karna (frontend ke liye)
    std::string toJSON() const {
        std::ostringstream os;
        os << "{"
           << "\"id\":"            << id << ","
           << "\"title\":\""       << escape(title)       << "\","
           << "\"description\":\"" << escape(description)  << "\","
           << "\"priority\":\""    << priorityToString(priority) << "\","
           << "\"status\":\""      << statusToString(status)     << "\""
           << "}";
        return os.str();
    }

private:
    // JSON ke liye basic string escaping (quotes/newlines safe)
    static std::string escape(const std::string& in) {
        std::string out;
        for (char c : in) {
            if (c == '"' || c == '\\') { out += '\\'; out += c; }
            else if (c == '\n') out += "\\n";
            else if (c == '\r') {}
            else out += c;
        }
        return out;
    }
};

// ============================================================================
//  "Board" class -> tasks ka collection manage karti hai.
//  (add / remove / move methods). Encapsulation bhi yahan use hua hai:
//  tasks vector private hai, aur mutex se thread-safe banaya hai.
// ============================================================================
class Board {
private:
    std::vector<std::unique_ptr<Task>> tasks;  // private collection
    int nextId = 1;
    std::mutex mtx;   // multithreaded server ke liye safety

public:
    // ---- ADD: naya task banao ----
    void addTask(const std::string& title, const std::string& desc, Priority p) {
        std::lock_guard<std::mutex> lock(mtx);
        tasks.push_back(std::make_unique<Task>(nextId++, title, desc, p));
    }

    // ---- REMOVE: id se task delete karo ----
    bool removeTask(int id) {
        std::lock_guard<std::mutex> lock(mtx);
        auto before = tasks.size();
        tasks.erase(std::remove_if(tasks.begin(), tasks.end(),
                    [id](const std::unique_ptr<Task>& t){ return t->getId() == id; }),
                    tasks.end());
        return tasks.size() != before;
    }

    // ---- MOVE: task ko doosre column (status) mein le jao ----
    bool moveTask(int id, Status newStatus) {
        std::lock_guard<std::mutex> lock(mtx);
        for (auto& t : tasks) {
            if (t->getId() == id) {
                t->setStatus(newStatus);   // setter se status badla (encapsulation)
                return true;
            }
        }
        return false;
    }

    // ---- Saare tasks ka JSON array banao (frontend isko render karega) ----
    std::string toJSON() {
        std::lock_guard<std::mutex> lock(mtx);
        std::ostringstream os;
        os << "[";
        for (size_t i = 0; i < tasks.size(); ++i) {
            if (i) os << ",";
            os << tasks[i]->toJSON();
        }
        os << "]";
        return os.str();
    }
};

// ============================================================================
//  ===== EMBEDDED FRONTEND =====
//  Pura HTML + CSS + JavaScript ek hi C++ raw-string ke andar.
//  Isse poora project EK .cpp file mein rehta hai.
// ============================================================================
static const std::string PAGE_HTML = R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Kanban Project Management System</title>
<style>
  * { box-sizing: border-box; margin: 0; padding: 0; }
  body { font-family: 'Segoe UI', Arial, sans-serif; background: #1b2638; color: #eaeef5; padding: 24px; }
  h1 { text-align: center; margin-bottom: 6px; font-weight: 600; }
  .subtitle { text-align:center; color:#8da2c0; margin-bottom: 20px; font-size: 14px; }

  /* ---- Add task form ---- */
  .form { background:#243246; padding:16px; border-radius:12px; max-width:760px; margin:0 auto 26px;
          display:flex; gap:10px; flex-wrap:wrap; align-items:center; }
  .form input, .form select { padding:10px; border-radius:8px; border:1px solid #3b4d68;
          background:#1b2638; color:#eaeef5; font-size:14px; }
  .form input#title { flex:1; min-width:160px; }
  .form input#desc  { flex:2; min-width:200px; }
  .form button { padding:10px 18px; border:none; border-radius:8px; background:#4f8cff; color:#fff;
          font-weight:600; cursor:pointer; }
  .form button:hover { background:#3a7bff; }

  /* ---- Board columns ---- */
  .board { display:flex; gap:18px; justify-content:center; align-items:flex-start; flex-wrap:wrap; }
  .column { background:#243246; border-radius:12px; width:320px; min-height:200px; padding:14px; }
  .column h2 { font-size:16px; margin-bottom:12px; padding-bottom:8px; border-bottom:2px solid #3b4d68; }
  .col-count { float:right; background:#3b4d68; border-radius:10px; padding:1px 9px; font-size:12px; }

  /* ---- Task card ---- */
  .card { background:#1b2638; border-radius:10px; padding:12px; margin-bottom:12px;
          border-left:5px solid #888; cursor:grab; }
  .card.dragging { opacity:0.4; }
  .card .ttl { font-weight:600; margin-bottom:4px; }
  .card .desc { font-size:13px; color:#a9bcd8; margin-bottom:8px; word-wrap:break-word; }
  .badge { font-size:11px; font-weight:700; padding:2px 9px; border-radius:10px; color:#fff; }

  /* priority colors */
  .Low    { border-left-color:#36c275; }
  .Medium { border-left-color:#f0ad33; }
  .High   { border-left-color:#e64949; }
  .badge.Low    { background:#36c275; }
  .badge.Medium { background:#f0ad33; }
  .badge.High   { background:#e64949; }

  .actions { margin-top:8px; display:flex; gap:6px; flex-wrap:wrap; }
  .actions button { font-size:11px; padding:4px 8px; border:none; border-radius:6px; cursor:pointer;
          background:#3b4d68; color:#eaeef5; }
  .actions button.del { background:#e64949; }
  .actions button:hover { opacity:0.85; }
  .column.over { outline:2px dashed #4f8cff; }
</style>
</head>
<body>
  <h1>Kanban Project Management System</h1>
  <div class="subtitle">C++ OOP Project &middot; tasks ko drag karke ya buttons se move karein</div>

  <div class="form">
    <input id="title" placeholder="Task title" />
    <input id="desc"  placeholder="Description" />
    <select id="priority">
      <option value="Low">Low</option>
      <option value="Medium" selected>Medium</option>
      <option value="High">High</option>
    </select>
    <button onclick="addTask()">+ Add Task</button>
  </div>

  <div class="board">
    <div class="column" id="todo"       ondrop="drop(event,'todo')"       ondragover="allow(event)" ondragleave="leave(event)">
      <h2>To Do <span class="col-count" id="c-todo">0</span></h2><div class="list" id="list-todo"></div>
    </div>
    <div class="column" id="inprogress" ondrop="drop(event,'inprogress')" ondragover="allow(event)" ondragleave="leave(event)">
      <h2>In Progress <span class="col-count" id="c-inprogress">0</span></h2><div class="list" id="list-inprogress"></div>
    </div>
    <div class="column" id="done"       ondrop="drop(event,'done')"       ondragover="allow(event)" ondragleave="leave(event)">
      <h2>Done <span class="col-count" id="c-done">0</span></h2><div class="list" id="list-done"></div>
    </div>
  </div>

<script>
  const cols = ['todo','inprogress','done'];

  // server se saare tasks fetch karke board render karo
  async function load() {
    const res = await fetch('/api/tasks');
    const tasks = await res.json();
    cols.forEach(c => document.getElementById('list-'+c).innerHTML = '');
    const counts = {todo:0, inprogress:0, done:0};
    tasks.forEach(t => {
      counts[t.status]++;
      document.getElementById('list-'+t.status).appendChild(card(t));
    });
    cols.forEach(c => document.getElementById('c-'+c).innerText = counts[c]);
  }

  // ek task ka HTML card banao
  function card(t) {
    const d = document.createElement('div');
    d.className = 'card ' + t.priority;
    d.draggable = true;
    d.dataset.id = t.id;
    d.ondragstart = e => { e.dataTransfer.setData('id', t.id); d.classList.add('dragging'); };
    d.ondragend   = () => d.classList.remove('dragging');
    d.innerHTML =
      '<div class="ttl">' + esc(t.title) + '</div>' +
      (t.description ? '<div class="desc">' + esc(t.description) + '</div>' : '') +
      '<span class="badge ' + t.priority + '">' + t.priority + '</span>' +
      '<div class="actions">' +
        (t.status!=='todo'       ? '<button onclick="move('+t.id+",'todo')\">&larr; To Do</button>" : '') +
        (t.status!=='inprogress' ? '<button onclick="move('+t.id+",'inprogress')\">In Progress</button>" : '') +
        (t.status!=='done'       ? '<button onclick="move('+t.id+",'done')\">Done &rarr;</button>" : '') +
        '<button class="del" onclick="del('+t.id+')">Delete</button>' +
      '</div>';
    return d;
  }

  function esc(s){ return (s||'').replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;'); }

  async function addTask() {
    const title = document.getElementById('title').value.trim();
    if (!title) { alert('Title likhna zaroori hai'); return; }
    const desc = document.getElementById('desc').value.trim();
    const priority = document.getElementById('priority').value;
    await fetch('/api/add', { method:'POST',
      body: JSON.stringify({title, description:desc, priority}) });
    document.getElementById('title').value = '';
    document.getElementById('desc').value  = '';
    load();
  }

  async function move(id, status) {
    await fetch('/api/move', { method:'POST', body: JSON.stringify({id, status}) });
    load();
  }
  async function del(id) {
    await fetch('/api/delete', { method:'POST', body: JSON.stringify({id}) });
    load();
  }

  // ---- drag & drop handlers ----
  function allow(e){ e.preventDefault(); e.currentTarget.classList.add('over'); }
  function leave(e){ e.currentTarget.classList.remove('over'); }
  function drop(e, status){
    e.preventDefault();
    e.currentTarget.classList.remove('over');
    const id = e.dataTransfer.getData('id');
    if (id) move(parseInt(id), status);
  }

  load();                 // page khulte hi load
  setInterval(load, 4000); // har 4 sec refresh (optional)
</script>
</body>
</html>)HTML";

// ============================================================================
//  "Server" class -> HTTP requests handle karti hai aur HTML/JSON serve karti.
//  Ye Board ko use karti hai (composition). Sockets yahan use hue hain.
// ============================================================================
class Server {
private:
    int port;
    Board& board;   // server board pe operate karta hai

    // ---- request body se ek field ki value nikaalna (basic JSON parse) ----
    static std::string getField(const std::string& body, const std::string& key) {
        std::string pat = "\"" + key + "\"";
        size_t k = body.find(pat);
        if (k == std::string::npos) return "";
        size_t colon = body.find(':', k);
        if (colon == std::string::npos) return "";
        size_t i = colon + 1;
        while (i < body.size() && (body[i]==' '||body[i]=='\t')) i++;
        if (i < body.size() && body[i] == '"') {       // string value
            size_t end = ++i;
            std::string out;
            while (end < body.size() && body[end] != '"') {
                if (body[end]=='\\' && end+1<body.size()) end++;  // escape skip
                out += body[end++];
            }
            return out;
        } else {                                        // number value
            size_t end = i;
            while (end < body.size() && (isdigit(body[end])||body[end]=='-')) end++;
            return body.substr(i, end - i);
        }
    }

    static Priority parsePriority(const std::string& s) {
        if (s == "High")   return Priority::High;
        if (s == "Low")    return Priority::Low;
        return Priority::Medium;
    }
    static Status parseStatus(const std::string& s) {
        if (s == "inprogress") return Status::InProgress;
        if (s == "done")       return Status::Done;
        return Status::ToDo;
    }

    // ---- HTTP response banane ka helper ----
    static std::string httpResponse(const std::string& body,
                                    const std::string& type = "text/html") {
        std::ostringstream os;
        os << "HTTP/1.1 200 OK\r\n"
           << "Content-Type: " << type << "\r\n"
           << "Content-Length: " << body.size() << "\r\n"
           << "Access-Control-Allow-Origin: *\r\n"
           << "Connection: close\r\n\r\n"
           << body;
        return os.str();
    }

    // ---- ek client connection handle karo (routing) ----
    void handleClient(int clientFd) {
        char buffer[8192];
        int n = recv(clientFd, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0) { close(clientFd); return; }
        buffer[n] = '\0';
        std::string req(buffer);

        // request line se method + path nikaalo
        std::string method = req.substr(0, req.find(' '));
        size_t p1 = req.find(' ');
        size_t p2 = req.find(' ', p1 + 1);
        std::string path = req.substr(p1 + 1, p2 - p1 - 1);

        // body (POST data) -> headers ke baad
        std::string body;
        size_t bpos = req.find("\r\n\r\n");
        if (bpos != std::string::npos) body = req.substr(bpos + 4);

        std::string response;

        // -------- ROUTING --------
        if (path == "/" || path == "/index.html") {
            response = httpResponse(PAGE_HTML, "text/html");
        }
        else if (path == "/api/tasks") {
            response = httpResponse(board.toJSON(), "application/json");
        }
        else if (path == "/api/add") {
            std::string title = getField(body, "title");
            std::string desc  = getField(body, "description");
            Priority pr       = parsePriority(getField(body, "priority"));
            if (!title.empty()) board.addTask(title, desc, pr);
            response = httpResponse("{\"ok\":true}", "application/json");
        }
        else if (path == "/api/move") {
            int id = atoi(getField(body, "id").c_str());
            board.moveTask(id, parseStatus(getField(body, "status")));
            response = httpResponse("{\"ok\":true}", "application/json");
        }
        else if (path == "/api/delete") {
            int id = atoi(getField(body, "id").c_str());
            board.removeTask(id);
            response = httpResponse("{\"ok\":true}", "application/json");
        }
        else {
            response = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
        }

        send(clientFd, response.c_str(), response.size(), 0);
        close(clientFd);
    }

public:
    Server(int port, Board& board) : port(port), board(board) {}

    // ---- server start: socket banao, bind, listen, accept loop ----
    void start() {
        int serverFd = socket(AF_INET, SOCK_STREAM, 0);
        if (serverFd < 0) { std::cerr << "socket() fail\n"; return; }

        int opt = 1;
        setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);

        if (bind(serverFd, (sockaddr*)&addr, sizeof(addr)) < 0) {
            std::cerr << "bind() fail. Port " << port << " busy ho sakta hai.\n";
            return;
        }
        listen(serverFd, 16);

        std::cout << "==============================================\n"
                  << "  Kanban Project Management System chal raha hai\n"
                  << "  Browser mein kholo:  http://localhost:" << port << "\n"
                  << "  Band karne ke liye:  Ctrl + C\n"
                  << "==============================================\n";

        // accept loop -> har request ko handle karo
        while (true) {
            sockaddr_in clientAddr{};
            socklen_t len = sizeof(clientAddr);
            int clientFd = accept(serverFd, (sockaddr*)&clientAddr, &len);
            if (clientFd < 0) continue;
            handleClient(clientFd);
        }
        close(serverFd);
    }
};

// ============================================================================
//  main()  -  sab kuch yahan se shuru
// ============================================================================
int main() {
    Board board;   // ek board banao

    // ---- demo ke liye kuch sample tasks (taaki board khaali na lage) ----
    board.addTask("Project setup", "Repo aur tools ready karo", Priority::High);
    board.addTask("Design UI",     "Kanban layout banao",        Priority::Medium);
    board.addTask("Write report",  "OOP concepts document karo", Priority::Low);

    Server server(8080, board);
    server.start();   // server start (yahan program ruk jata hai loop mein)
    return 0;
}
