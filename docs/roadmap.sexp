; The yojimbo roadmap: the data ROADMAP.md is generated from.
; Same shape as nova-sprint's docs/roadmap.sexp (internal/roadmapdoc's package comment):
; data, never evaluated. Edit this file, then regenerate ROADMAP.md with nova-sprint's
; generator, from a nova-sprint checkout:
;   go build -o /tmp/roadmap ./tools/roadmap && (cd <this repo> && /tmp/roadmap --file docs/roadmap.sexp --out ROADMAP.md)
; Never edit ROADMAP.md by hand. Created 2026-10-14 when this repository's open GitHub issues and
; unfinished pull requests were moved into it.
(roadmap "v1"
 :title "yojimbo roadmap"
 :text "Planned work for yojimbo. The items below were open GitHub issues and pull requests, moved here on 2026-10-14 when roadmaps became the standard for future work (Glenn, 2026-10-14). Each item names its issue or pull request; the branches of moved pull requests are kept to resume from. Landed work is in git."

 :groups
 (
  (group "github-issues" :title "Moved from GitHub issues on 2026-10-14"
   :text "Open GitHub issues of mas-bandwidth/yojimbo, recorded here on 2026-10-14 when roadmaps became the standard for future work (Glenn, 2026-10-14). Each item names its issue; the issue is closed with a pointer here.")
 )

 :items
 (
  (item "issue-306" :group "github-issues" :area "yojimbo"
   :title "Would you sign the Contributor Assignment Agreement?"
   :text "Hi, and thank you for contributing to yojimbo over the years. I'm asking contributors to sign a short Contributor Assignment Agreement (CAA). New contributions already require it, and I'd like past contributors to sign it too, so ownership of yojimbo is consolidated and I can keep licensing and maintaining it cleanly."
   :date "2026-10-14"
   :origin "mas-bandwidth/yojimbo issue #306, opened 2026-07-18 by gafferongames; moved here 2026-10-14"
   :kept (:github-issue "306" :labels ""))
  (item "issue-331" :group "github-issues" :area "yojimbo"
   :title "Would you sign the Contributor Assignment Agreement?"
   :text "Hi, and thank you for contributing to yojimbo over the years. I'm asking contributors to sign a short Contributor Assignment Agreement (CAA). New contributions already require it, and I'd like past contributors to sign it too, so ownership of yojimbo is consolidated and I can keep licensing and maintaining it cleanly."
   :date "2026-10-14"
   :origin "mas-bandwidth/yojimbo issue #331, opened 2026-08-23 by gafferongames; moved here 2026-10-14"
   :kept (:github-issue "331" :labels ""))
  (item "issue-339" :group "github-issues" :area "yojimbo"
   :title "Security and quality audit of yojimbo, netcode and reliable, 2026-09-04 (record; SEC-01 withheld to a private advisory)"
   :text "Glenn shared this audit of yojimbo, netcode and reliable on 2026-09-04 to be worked on later. Recorded verbatim except SEC-01, which is withheld to a private draft security advisory on netcode (https://github.com/mas-bandwidth/netcode/security/advisories/GHSA-v29p-3vj4-vg4f) per the audit's own handling note."
   :date "2026-10-14"
   :origin "mas-bandwidth/yojimbo issue #339, opened 2026-09-04 by gafferongames; moved here 2026-10-14"
   :kept (:github-issue "339" :labels ""))
  (item "issue-341" :group "github-issues" :area "yojimbo"
   :title "Document every public declaration, then turn WARN_IF_UNDOCUMENTED on"
   :text "Document every public declaration, then turn WARN_IF_UNDOCUMENTED on"
   :date "2026-10-14"
   :origin "mas-bandwidth/yojimbo issue #341, opened 2026-09-04 by gafferongames; moved here 2026-10-14"
   :kept (:github-issue "341" :labels ""))
  (item "issue-344" :group "github-issues" :area "yojimbo"
   :title "connect handshake carries a schema vocabulary announcement"
   :text "From the schema message form (schema#530 and its second round, owner rulings 2026-09-05): a table-wire message body rides without its id table when the peer has announced its unit's vocabulary once, reliably, before the first body, and never re-announces for the connection's life."
   :date "2026-10-14"
   :origin "mas-bandwidth/yojimbo issue #344, opened 2026-09-05 by gafferongames; moved here 2026-10-14"
   :kept (:github-issue "344" :labels ""))
 ))
