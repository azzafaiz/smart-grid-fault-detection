# ⚡ Smart Grid Fault Detection and Automatic Rerouting System

This project implements a **Smart Grid Fault Detection and Automatic Rerouting System** that continuously monitors electrical lines, detects faults in real time, isolates affected sections, and restores power through alternate paths. The system uses an **ESP8266-based control unit**, relay switching, and a **web-based monitoring dashboard** integrated with a real-time cloud database.

---

## 🔍 Problem Overview
Traditional power distribution systems depend largely on manual fault identification and restoration. This leads to delayed response, extended power outages, and reduced grid reliability. With increasing demand for uninterrupted power, there is a need for an automated system capable of monitoring grid conditions and responding instantly to faults.

---

## 💡 Solution Approach
The proposed system introduces intelligence into the distribution network by continuously monitoring current levels at multiple grid nodes. When an abnormal condition is detected, the system automatically isolates the faulty section and reroutes power through a healthy alternate path. A web-based dashboard provides real-time visualization and remote control capability.

---

## 🧠 System Working
1. Current sensors measure line current at each grid node  
2. The ESP8266 processes sensor data in real time  
3. Faults are detected using predefined threshold conditions  
4. Relays isolate the faulty section of the grid  
5. Power is rerouted through alternate healthy paths  
6. All status updates are sent to a cloud database  
7. The dashboard displays live grid status and relay states  

---

## 🌐 Monitoring Dashboard
The web dashboard provides:
- Live grid visualization  
- Color-coded line status (Normal / Fault)  
- Real-time current values at each node  
- Relay ON/OFF status  
- Manual switching control  
- Fault and event logs  

This enables effective monitoring and control of the grid from any location.

---

## 🛠️ Technologies Used

### Hardware
- ESP8266 (Wi-Fi enabled microcontroller)  
- Relay modules  
- Current sensors  
- Power supply unit  

### Software
- Embedded firmware  
- HTML, CSS, JavaScript  
- Firebase Realtime Database  
- VS Code

- ---

## 📈 Results
- Immediate fault detection  
- Automatic power rerouting  
- Stable system operation  
- Real-time monitoring and control  
- Reduced outage duration  

---

## ✅ Advantages
- Faster fault response  
- Reduced manual intervention  
- Improved grid reliability  
- Remote monitoring capability  
- Scalable and cost-effective design  

---

## ⚠️ Limitations
- Threshold-based fault detection  
- Limited number of grid nodes  
- Internet dependency  
- No predictive fault analysis  

---

## 🔮 Future Enhancements
- AI-based fault prediction  
- Mobile application integration  
- Power quality monitoring  
- Large-scale grid expansion  
- Integration with advanced grid management systems  

---

## 👤 Author
**Azza Faiz**  
B.Tech Electrical Engineering

---
