# nanoprintf

[![CircleCI](https://circleci.com/gh/charlesnicholson/nanoprintf.svg?style=shield)](https://circleci.com/gh/charlesnicholson/nanoprintf) [![](https://img.shields.io/badge/license-public_domain-brightgreen.svg)](https://github.com/charlesnicholson/nanoprintf/blob/master/LICENSE)

nanoprintf is an almost-standard-compliant implementation of snprintf and vsnprintf for embedded systems. Zero memory allocations, less than 100 bytes of stack, less than 5kb object code (cortex-m, -Os) with the bells and whistles turned on.
