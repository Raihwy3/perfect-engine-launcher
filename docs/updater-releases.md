# Preparing updater releases

For release maintainers: use tags such as `v0.2.4` or `v0.2.4-r1` and attach
`perfect-engine-launcher.zip` with the `3ds/perfect-engine-launcher/` layout.
The updater verifies HTTPS, the GitHub asset SHA-256 digest, file size and ZIP
checksums, and keeps a recovery journal while replacing the two files.
Drafts and GitHub prereleases are skipped. Console update checks need a public
repository and a published release; private releases are not accessible.
