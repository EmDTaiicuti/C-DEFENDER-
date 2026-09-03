# C-DEFENDER: 2D TOWER DEFENSE GAME

> **Đồ án môn học:** Cấu trúc Dữ liệu và Giải thuật (DSA)  
> **Ngôn ngữ:** C++ 
> **Thư viện đồ họa:** Raylib  
> **Repository:** [https://github.com/EmDTaiicuti/C-DEFENDER-](https://github.com/EmDTaiicuti/C-DEFENDER-)

---

## 1. Giới thiệu Đề tài
**C-DEFENDER** là một tựa game chiến thuật thủ thành (Tower Defense) 2D được xây dựng hoàn toàn bằng C++ kết hợp thư viện đồ họa Raylib. Trong game, người chơi có nhiệm vụ bố trí các tháp phòng thủ chiến lược dọc bản đồ để ngăn chặn các đợt quái vật xâm chiếm nhà chính.

Dự án tập trung giải quyết bài toán tối ưu hóa tìm đường, điều phối luồng thực thi và quản lý tài nguyên trong game thông qua các cấu trúc dữ liệu và giải thuật nâng cao.

---

## 2. Cấu trúc Dữ liệu & Giải thuật áp dụng (DSA Core)

| Cấu trúc / Thuật toán | Vị trí áp dụng | Mục đích & Chi tiết |
| :--- | :--- | :--- |
| **Thuật toán $A^*$ (A-Star)** | `MapManager` / Quái vật | Sử dụng heuristic khoảng cách Manhattan để tính toán lộ trình di chuyển ngắn nhất cho quái vật qua bản đồ lưới (Grid Map). |
| **Hàng đợi ưu tiên (Priority Queue / Min-Heap)** | Tìm đường & Quản lý hiệu ứng | Chọn node có chi phí $f(n)$ nhỏ nhất trong $A^*$, đồng thời quản lý thời gian hết hạn của các hiệu ứng debuff (Làm chậm, Thiêu đốt). |
| **Hàng đợi (Queue - FIFO)** | `WaveManager` | Quản lý thứ tự xuất hiện của các đợt quái (Wave 1, Wave 2...) và chu kỳ nhả lính. |
| **Ngăn xếp (Stack - LIFO)** | `GameManager` | Hỗ trợ tính năng Hoàn tác / Đi lại (**Undo/Redo**) khi người chơi mua nhầm hoặc bán trụ. |
| **Cây tứ phân (QuadTree)** | Hệ thống Đạn & Va chạm | Phân vùng không gian 2D, giảm độ phức tạp tính toán va chạm giữa quái vật và đạn từ $O(N \times M)$ xuống $O(\log N)$. |
| **Danh sách liên kết / Vector** | Quản lý Thực thể | Quản lý vòng đời cấp phát và giải phóng của quái vật (`Enemy`) và đạn (`Bullet`) trên sàn đấu. |

---

## 3. Kiến trúc Lập trình Hướng đối tượng (OOP)

Dự án áp dụng chặt chẽ 4 tính chất OOP trong C++:

*   **Tính đóng gói (Encapsulation):** Toàn bộ dữ liệu trạng thái (chỉ số máu, tầm đánh, vị trí) được bảo vệ bằng phạm vi truy cập `private`/`protected`, giao tiếp qua các phương thức getter/setter.
*   **Tính kế thừa (Inheritance):**
    *   `Enemy` (Class cơ sở) $\rightarrow$ `Goblin` (chạy nhanh), `Golem` (máu trâu), `Boss`.
    *   `Tower` (Class cơ sở) $\rightarrow$ `ArcherTower` (bắn đơn), `CannonTower` (bắn lan AoE), `IceTower` (làm chậm).
    *   `Bullet` $\rightarrow$ `NormalBullet`, `ExplosiveBullet`.
*   **Tính đa hình (Polymorphism):** Sử dụng các phương thức ảo thuần túy (`virtual void Update() = 0`, `virtual void Draw() = 0`) để `GameManager` quản lý mảng con trỏ lớp cha một cách đồng nhất.
*   **Tính trừu tượng (Abstraction):** Tách biệt tầng logic tính toán (thời gian hồi chiêu, sát thương, tìm mục tiêu) và tầng hiển thị đồ họa Raylib.

---

## 4. Cấu trúc Thư mục

```text
C-DEFENDER-/
├── assets/                 # Sprite nhân vật, quái vật, map, âm thanh (.png, .wav)
├── include/                # Header files (.hpp)
│   ├── Common.hpp          # Struct tọa độ, enum trạng thái
│   ├── MapManager.hpp      # Ma trận bản đồ, thuật toán A*
│   ├── Enemy.hpp           # Hệ thống lớp Enemy (Kế thừa)
│   ├── Tower.hpp           # Hệ thống lớp Tower & Targeting
│   ├── Bullet.hpp          # Cơ chế bắn và tính sát thương
│   ├── WaveManager.hpp     # Quản lý đợt quái bằng Queue
│   ├── QuadTree.hpp        # Cây tứ phân tối ưu va chạm
│   └── GameManager.hpp     # Game Loop, Raylib UI, Undo Stack
├── src/                    # Implementation files (.cpp)
│   ├── main.cpp
│   ├── MapManager.cpp
│   ├── Enemy.cpp
│   ├── Tower.cpp
│   ├── Bullet.cpp
│   ├── WaveManager.cpp
│   ├── QuadTree.cpp
│   └── GameManager.cpp
└── README.md
