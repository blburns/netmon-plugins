# Automation

This directory contains CI/CD automation configurations.

## CI/CD

- `ci/` - Continuous Integration configuration (Jenkins, GitHub Actions, etc.)
- `docker/` - Docker build and deployment configurations
- `vagrant/` - Vagrant development environment setup

## Docker

Build and test plugins in a containerized environment:

```bash
cd automation/docker
docker-compose up
```

## CI Configuration

CI pipelines are configured to:
- Build on multiple platforms
- Run test suites
- Generate packages
- Deploy artifacts

