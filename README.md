`db.h` and `db.cpp` files are designed to work with Microsoft SQL Server and offers a clean C++ abstraction for SQL interaction in a web service.

### ✅ Purpose:

These files implement a lightweight database access layer using **ODBC** to connect to an SQL Server database. They include helper classes to:

* Escape/encode strings for **HTML** and **JSON**
* Execute SQL commands and return results
* Format SQL result sets into HTML tables or JSON arrays

---

### 🔧 Key Components:

#### 1. `Convert`

* Provides static methods for:

  * Escaping special characters for JSON (`JsonEscape`)
  * HTML encoding (`HtmlEncode`)

#### 2. `DbColumn`

* Holds metadata for a SQL result column (name, type, size, etc.)

#### 3. `DbReader`

* Reads data row-by-row from a SQL statement handle.
* Provides typed getters (`GetInt32`, `GetString`, `GetBoolean`, etc.)
* Can extract column metadata via `GetColumnsInfo`

#### 4. `DbClient`

* Manages connection to the database via ODBC
* Core methods:

  * `Connect` / `Disconnect`
  * `ExecuteReader`: Executes a query and invokes a callback for row processing
  * `ExecuteHtmlTable`: Returns an HTML table for a query
  * `ExecuteJsonObject`: Returns a JSON array of rows
  * `ExecuteScalarInt` / `ExecuteScalarString`: Runs scalar queries
  * `ExecuteNonQuery`: Executes SQL commands like INSERT/UPDATE/DELETE

Here are concise summaries of each class in the provided Web namespace inside both web.h and web.cpp

### **1. `HttpRequest`**
- **Purpose**: Parses and stores HTTP request data.
- **Key Properties**:
  - `method`: HTTP method (e.g., "GET", "POST").
  - `url`: Request path (e.g., "/index.html").
  - `headers`/`cookies`/`queryParams`: Key-value pairs from the request.
  - `body`: Raw request body (for POST/PUT).
- **Key Methods**:
  - `getHeader()` / `getCookie()` / `getQueryParam()`: Retrieve specific values.
- **Notes**:
  - Parses raw HTTP requests (e.g., extracts headers, query strings).
  - Includes the client `SOCKET` for response handling.

---

### **2. `StaticFileHandler`**
- **Purpose**: Serves static files (e.g., HTML, CSS, JS).
- **Key Methods**:
  - `TryHandleRequest()`: Checks if the request is for a file; serves it if found.
  - `SendFile()`: Reads a file and sends it via the socket.
- **Notes**:
  - Converts URLs to filesystem paths (`ConvertToPathWindows`).
  - Sets `Content-Type` based on file extension (`GetContentType`).

---

### **3. `HttpResponse`**
- **Purpose**: Constructs and sends HTTP responses.
- **Key Properties**:
  - `StatusCode` (e.g., 200, 404).
  - `ContentType` (e.g., "text/html").
  - `Body`: Response content.
- **Key Methods**:
  - `SetHeader()`: Adds custom headers.
  - `Redirect()`: Sends a 302 redirect.
  - `ToString()`: Formats the response as a valid HTTP message.
- **Notes**:
  - Uses the client `SOCKET` from `HttpRequest` to send data.

---

### **4. `HttpContext`**
- **Purpose**: Bundles request/response and route data for handlers.
- **Key Properties**:
  - `Request`: The incoming `HttpRequest`.
  - `Response`: The outgoing `HttpResponse`.
  - `RouteData`: Dynamic route parameters (e.g., `/users/{id}` → `{"id": "123"}`).
- **Notes**:
  - Passed to route handlers for unified access to request/response.

---

### **5. `RoutePattern`**
- **Purpose**: Matches URL paths against route templates (e.g., `/users/{id}`).
- **Key Methods**:
  - `match()`: Checks if a path matches the pattern; extracts parameters into `routeValues`.
- **Notes**:
  - Segments like `{id}` are treated as dynamic parameters.

---

### **6. `Router`**
- **Purpose**: Manages route registration and dispatching.
- **Key Methods**:
  - `addRoute()`: Maps HTTP methods + patterns to handlers (e.g., `GET /users` → `UserController::list`).
  - `handleRequest()`: Matches a request to a route and invokes its handler.
- **Notes**:
  - Uses `RoutePattern` for path matching.
  - Handlers return a `std::string` (response body) and modify `HttpContext`.

---

### **7. `Server`**
- **Purpose**: Listens for HTTP connections and dispatches requests.
- **Key Methods**:
  - `start()` / `stop()`: Manages the server lifecycle.
  - `setRouter()`: Attaches a `Router` for request handling.
- **Workflow**:
  1. Accepts connections in `acceptLoop()`.
  2. Spawns threads (`handleClient()`) to process each request.
  3. Delegates routing to the `Router`.
- **Notes**:
  - Uses Winsock (`SOCKET`) for low-level networking.
  - Thread-per-connection model (scalability may be limited).

Here is a summary of the C++ MVC inside both mvc.h and mvc.cpp

* **PageBuilder**: Provides a static method to render a layout using specified style, body, and script sections.
* **FormCollection**: A dictionary-like container for form input (`std::map<string, string>`).
* **ActionResult**: An abstract base class for results that must implement a `ToString()` method.
* **View**: Represents an HTML page view with layout and content sections; derived from `ActionResult`.
* **JsonResult**: Represents a JSON response; also derived from `ActionResult`.
* **Controller**: Base class for web controllers, holding an `HttpContext` and a `View`.


