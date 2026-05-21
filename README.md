# 🔐 Smart Security Door Lock System

A professional multi-phase Arduino-based smart security system featuring RFID authentication, PIN verification, EEPROM persistence, role-based access control, lockout protection, and real hardware implementation.

Developed progressively through simulation and real-world deployment using Arduino platforms.

---

# 📌 Project Overview

This project demonstrates the complete development lifecycle of an embedded security system across three phases:

| Phase | Platform | Controller | Features |
|------|------|------|------|
| Phase 1 | Tinkercad | Arduino UNO | Basic Servo Lock Prototype |
| Phase 2 | Wokwi Simulation | Arduino Mega | RFID + PIN + LCD + EEPROM |
| Phase 3 | Real Hardware | Arduino UNO | Full Physical Smart Lock System |

The system evolved from a simple servo-controlled lock into a fully-featured smart security solution with authentication layers, admin controls, EEPROM persistence, and real hardware optimizations.

---

# ✨ Features

## 🔑 Authentication System
- RFID Card Authentication
- PIN Code Authentication
- Role-Based Access Control
- Admin Override System
- Panic Code Support (`911`)

## 🛡️ Security Features
- Failed Attempt Detection
- 30-Second Lockout System
- Alarm & Buzzer Feedback
- RFID Admin Recovery
- EEPROM Persistent Storage

## 🖥️ User Interface
- 16x2 I2C LCD Display
- Real-Time Status Messages
- Custom LCD Lock Icons
- Keypad Input System

## ⚙️ Hardware Features
- Servo-Based Smart Lock
- Smooth Servo Motion
- LED Status Indicators
- Sound Feedback System

---

# 🧠 System Architecture

The project uses layered authentication and embedded state-machine logic for security and reliability.

## Main Components
- Arduino UNO / Mega
- RFID RC522 Module
- 16x2 I2C LCD
- 3x4 / 4x4 Keypad
- Servo Motor
- LEDs
- Buzzer
- EEPROM Memory

---

# 📂 Project Structure

```bash
Smart-Security-Door-Lock-System/
│
├── Documentation/
│   └── SmartDoorLock_Documentation.docx
│
├── Images/
│   ├── phase1.png
│   ├── phase2.png
│   └── phase3.jpg
│
├── Phase1_Tinkercad/
│   └── phase1.ino
│
├── Phase2_Wokwi/
│   ├── phase2.ino
│   └── diagram.json
│
├── Phase3_RealHardware/
│   └── phase3.ino
│
├── README.md
├── LICENSE
└── .gitignore
```

---

# 🚀 Development Phases

# 🧪 Phase 1 — Tinkercad Prototype

The first phase validates the basic smart lock mechanism using:
- Push button input
- Servo motor locking system
- LED indicators
- Auto-lock functionality

## Features
- Servo Lock/Unlock
- Red/Green LED Status
- Auto Lock After Delay
- Serial Monitor Logging

---

# 💻 Phase 2 — Wokwi Full Simulation

The second phase expands the system into a complete simulated security platform using Arduino Mega.

## Features
- RFID Authentication
- PIN Verification
- EEPROM Persistence
- LCD Interface
- Admin Menu
- Multi-User Support
- Lockout Protection
- Panic Alarm

## User Roles
- Admin
- Standard User
- Guest

---

# 🔧 Phase 3 — Real Hardware Implementation

The final phase migrates the project to physical hardware using Arduino UNO with memory optimization and hardware adaptations.

## Additional Features
- Smooth Servo Movement
- RFID Lockout Override
- Dynamic RFID Registration
- LCD Custom Icons
- SRAM Optimization using `F()` Macro

---

# 🔄 State Machine Design

The system operates through multiple states:

| State | Description |
|------|------|
| `SYS_IDLE` | Waiting for input |
| `SYS_LOCKED` | Lockout mode |
| `SYS_ADMIN_LOGIN` | Admin authentication |
| `SYS_ADMIN_MENU` | Admin control panel |
| `SYS_ROLE_SELECT` | Select user role |
| `SYS_USER_EDIT` | Edit user settings |
| `SYS_CHANGE_PIN` | Change user PIN |

---

# 🧰 Technologies Used

## Hardware
- Arduino UNO
- Arduino Mega
- RFID RC522
- Servo Motor
- Keypad
- I2C LCD
- EEPROM

## Software & Libraries
- Arduino IDE
- C++
- SPI Library
- MFRC522 Library
- LiquidCrystal_I2C
- EEPROM Library
- Keypad Library

## Simulation Platforms
- Tinkercad
- Wokwi

---

# 🔌 Wiring & Communication

## Protocols Used
- SPI Communication → RFID RC522
- I2C Communication → LCD Display
- Digital I/O → LEDs, Keypad, Servo, Buzzer

---

# 📸 Project Preview

## Phase 1 — Prototype
![Phase1](Images/phase1.png)

## Phase 2 — Simulation
![Phase2](Images/phase2.png)

## Phase 3 — Real Hardware
![Phase3](Images/phase3.jpg)

---

# 🛠️ Installation & Usage

## 1️⃣ Clone Repository

```bash
git clone https://github.com/YOUR_USERNAME/Smart-Security-Door-Lock-System.git
```

---

## 2️⃣ Open Arduino IDE

Install required libraries:
- MFRC522
- LiquidCrystal_I2C
- Keypad

---

## 3️⃣ Upload Code

Choose desired phase:
- Phase 1 → Basic Prototype
- Phase 2 → Wokwi Simulation
- Phase 3 → Real Hardware

Upload `.ino` file to Arduino board.

---

# 📈 Future Improvements

- ESP32 WiFi Integration
- Mobile App Control
- Cloud Database Logging
- Biometric Fingerprint Sensor
- Face Recognition
- Camera Security Module
- OTA PIN Management
- 3D Printed Enclosure

---

# 🎯 Learning Outcomes

This project demonstrates:
- Embedded Systems Engineering
- Arduino Development
- Security System Design
- State Machine Architecture
- EEPROM Data Persistence
- RFID Authentication
- Hardware/Software Integration
- Real-Time System Design

---

# 👨‍💻 Author

## Youssef Ahmed

Embedded Systems & Arduino Developer

---

# 📄 License

This project is licensed under the MIT License.

---

# ⭐ Support

If you like this project:
- Star the repository
- Fork the project
- Share it with others

---

# 📬 Contact

Feel free to connect or contribute to the project.
