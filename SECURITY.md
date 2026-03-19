# Security Policy

## Supported Versions

We actively maintain and provide security updates for the following versions:

| Version | Supported          |
| ------- | ------------------ |
| 1.x.x   | Yes                |
| < 1.0   | No                 |

---

## Reporting a Vulnerability

If you discover a security vulnerability, please report it responsibly.

📧 Email: support@simdaas.com

Please include:

- Description of the vulnerability  
- Steps to reproduce  
- Affected components (e.g., driver, communication, RTOS module)  
- Possible impact  
- Suggested fix (if available)  

---

## Response Process

Once a vulnerability is reported:

1. We will acknowledge receipt within **48 hours**  
2. We will investigate and validate the issue  
3. A fix will be developed and tested  
4. A patch or update will be released  
5. The reporter may be credited (if desired)  

---

## Security Best Practices (For Contributors)

Since this project involves **embedded systems and hardware interaction**, contributors must:

- Avoid unsafe memory usage (buffer overflow, memory leaks)  
- Validate all inputs (UART, MQTT, sensors, etc.)  
- Handle communication securely (encryption where applicable)  
- Avoid hardcoded credentials or secrets  
- Ensure safe interaction with hardware peripherals  
- Consider real-time and concurrency issues (FreeRTOS, interrupts)  

---

## Disclosure Policy

- Do not publicly disclose vulnerabilities before they are fixed  
- Responsible disclosure is expected  
- We will coordinate with reporters before public announcements  

---

## Scope

This security policy applies to:

- Firmware code  
- Drivers and hardware interfaces  
- Communication modules (MQTT, GSM, LoRa, etc.)  
- RTOS-based components  

---

## Out of Scope

The following are generally not considered security vulnerabilities:

- Minor code style issues  
- Non-critical bugs without security impact  
- Theoretical issues without practical exploit  

---

## Final Note

Security is critical in embedded and autonomous systems.

> A small bug in software can become a big problem in hardware.

We appreciate your efforts in keeping **Simdaas Autonomy** secure 🔐