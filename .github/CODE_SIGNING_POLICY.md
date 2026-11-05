# Code signing policy

## Scope

Windows builds of Vikey published from this repository are code-signed: the
`Vikey.exe` binaries for x64 and arm64 that end up in the release assets
(standalone, and inside the portable zip / installer).

Other platforms and development builds are not signed.

## Build and signing process

- Binaries are built by GitHub Actions (`.github/workflows/windows.yml`) from
  the commit being released, on GitHub-hosted runners.
- No locally built or manually uploaded binary is signed.
- The signing request references the GitHub Actions artifact produced by the
  same workflow run, which lets SignPath verify the binary's origin.
- Signing is performed by [SignPath.io](https://signpath.io) with a
  certificate issued by the [SignPath Foundation](https://signpath.org).
  Private keys stay in SignPath's HSM and are never available to this project.
- Signing requests are only submitted by the release job of the tagged build
  (`v*` tags) and require the repository secret `SIGNPATH_API_TOKEN`.

## Roles

- Maintainer: [@antran02](https://github.com/antran02) — approves releases and
  maintains this policy.

## Verification

```powershell
Get-AuthenticodeSignature .\Vikey.exe | Format-List Status, SignerCertificate
```

or

```
signtool verify /pa /v Vikey.exe
```

A valid signature reports `Status: Valid` and a signer certificate issued to
the SignPath Foundation.

## Reporting a problem

Open an issue in this repository. For a suspected key compromise or a binary
that does not match the published SHA-256, report it immediately and do not run
the binary.
