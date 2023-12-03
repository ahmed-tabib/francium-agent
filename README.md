# Francium-agent: A PoC malware agent with XMR mining capabilities

Francium-agent is an open-source Proof-of-Concept agent for a monero mining network. The project is made as a display of skill only and is mostly non-functional out of the box, any malicious use is the responsibility of the user only.

## Features

- **Mining Controller**: A special controller for XMR mining with ability to fine tune CPU usage within ~3%, the only one of its kind with such capabilities
  
- **Stealth Capabilities**: Changes CPU and RAM usage based on user actions to remain undetected (mouse movements, open task manager, etc..)

- **C2 Capabilities**: Communicates with the server through a custom binary C2 Protocol.

## Getting Started

### Prerequisites

- Visual Studio 2019/2022

### Installation
```bash
git clone github.com/ahmed-tabib/francium-agent
```
Clone the repository and launch the project in VS 2022.

## Usage

Currently there is no way to use the project, unless you decide to integrate the agent into your C2 framework.

## Contributing

We welcome contributions from the community! If you find a bug, have a feature request, or would like to contribute code, please open an issue or submit a pull request.

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details.

## Acknowledgments

- [RandomX](https://github.com/tevador/RandomX) - Proof of work algorithm based on random code execution
