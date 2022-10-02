#!/bin/bash


git subrepo clone https://github.com/free-audio/clap-juce-extensions.git deps/clap-juce-extensions --force
git subrepo clone https://github.com/free-audio/clap.git deps/clap-juce-extensions/clap-libs/clap --force
git subrepo clone https://github.com/free-audio/clap-helpers.git deps/clap-juce-extensions/clap-libs/clap-helpers --force
