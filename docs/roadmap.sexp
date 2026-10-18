; yojimbo roadmap: the data ROADMAP.md is generated from (nova-tools tools/roadmap;
; make roadmap renders, make roadmap-check fails on drift). Read as data, never evaluated.
; A cell is done only when its task is :done with :evidence, a test or a commit;
; :tested-revision is the main commit whose CI passed with that evidence.
(:schema 1
 :root "goal"
 :page (:title "yojimbo roadmap"
        :roadmaps ("connection" "messages" "network" "library" "testing"))
 :nodes
 ((:id "goal" :type :work-set :children ("connection" "messages" "network" "library" "testing"))
  (:id "connection" :type :roadmap :title "Connection"
   :text "Secure connect tokens, connecting, disconnecting and timeouts between a client and a server."
   :rows (("connect-tokens" "secure connect with a connect token")
          ("token-single-use" "a connect token admits one connection only")
          ("client-server" "client/server connection management")
          ("timeouts" "connection request and connection timeouts")
          ("disconnect-reasons" "disconnects carry a reason, on both sides")
          ("state-machine" "client and server states match STATE-MACHINE.md")
          ("loopback" "loopback clients inside the server process")
          ("schema-vocabulary" "connect handshake carries a schema vocabulary announcement"))
   :columns (("done" "done"))
   :cells (("connect-tokens" "done" "connection/connect-tokens")
           ("token-single-use" "done" "connection/token-single-use")
           ("client-server" "done" "connection/client-server")
           ("timeouts" "done" "connection/timeouts")
           ("disconnect-reasons" "done" "connection/disconnect-reasons")
           ("state-machine" "done" "connection/state-machine")
           ("loopback" "done" "connection/loopback")
           ("schema-vocabulary" "done" "connection/schema-vocabulary")))
  (:id "connection/connect-tokens" :type :task :title "secure connect with a connect token" :state :done :evidence ("test.cpp: test_client_connect_twice_with_one_connect_token" ".github/workflows/ci.yml: conformance job, Verify STATE-MACHINE.md") :tested-revision "3bdbaf2b5a669083c4dcf8d77a939e063567468b")
  (:id "connection/token-single-use" :type :task :title "a connect token admits one connection only" :state :done :evidence ("test.cpp: test_client_connect_twice_with_one_connect_token" "commit a1eb09e (yojimbo 1.12.1: vendor netcode 1.4.5 and reliable 1.4.2)") :tested-revision "3bdbaf2b5a669083c4dcf8d77a939e063567468b")
  (:id "connection/client-server" :type :task :title "client/server connection management" :state :done :evidence ("test.cpp: test_client_server_messages" "test.cpp: test_client_server_start_stop_restart") :tested-revision "3bdbaf2b5a669083c4dcf8d77a939e063567468b")
  (:id "connection/timeouts" :type :task :title "connection request and connection timeouts" :state :done :evidence ("test.cpp: test_client_connect_twice_with_one_connect_token") :tested-revision "3bdbaf2b5a669083c4dcf8d77a939e063567468b")
  (:id "connection/disconnect-reasons" :type :task :title "disconnects carry a reason, on both sides" :state :done :evidence ("test.cpp: test_server_client_disconnect_reason" "test.cpp: test_client_disconnect_reason") :tested-revision "3bdbaf2b5a669083c4dcf8d77a939e063567468b")
  (:id "connection/state-machine" :type :task :title "client and server states match STATE-MACHINE.md" :state :done :evidence (".github/workflows/ci.yml: conformance job, Verify STATE-MACHINE.md") :tested-revision "3bdbaf2b5a669083c4dcf8d77a939e063567468b")
  (:id "connection/loopback" :type :task :title "loopback clients inside the server process" :state :unknown
   :note "Only test_client_is_loopback_when_disconnected and a link check call the loopback API; a test that connects a loopback client and exchanges messages with it makes this done.")
  (:id "connection/schema-vocabulary" :type :task :title "connect handshake carries a schema vocabulary announcement" :state :todo
   :note "From the schema message form (schema#530 and its second round, owner rulings 2026-09-05): a table-wire message body rides without its id table when the peer has announced its unit's vocabulary once, reliably, before the first body, and never re-announces for the connection's life."
   :github-issue "344")
  (:id "messages" :type :roadmap :title "Messages"
   :text "Channels of messages and data blocks sent over the connection."
   :rows (("reliable-ordered" "reliable-ordered messages, resent until acked")
          ("unreliable-unordered" "unreliable-unordered messages for time-sensitive data")
          ("blocks" "data blocks larger than a packet on reliable-ordered messages")
          ("unreliable-blocks" "data blocks on unreliable-unordered messages")
          ("multiple-channels" "multiple channels on one connection")
          ("serialize" "bitpacker and serialization system")
          ("queue-limits" "full send and receive queues fail cleanly"))
   :columns (("done" "done"))
   :cells (("reliable-ordered" "done" "messages/reliable-ordered")
           ("unreliable-unordered" "done" "messages/unreliable-unordered")
           ("blocks" "done" "messages/blocks")
           ("unreliable-blocks" "done" "messages/unreliable-blocks")
           ("multiple-channels" "done" "messages/multiple-channels")
           ("serialize" "done" "messages/serialize")
           ("queue-limits" "done" "messages/queue-limits")))
  (:id "messages/reliable-ordered" :type :task :title "reliable-ordered messages, resent until acked" :state :done :evidence ("test.cpp: test_connection_reliable_ordered_messages" "test.cpp: test_single_message_type_reliable") :tested-revision "3bdbaf2b5a669083c4dcf8d77a939e063567468b")
  (:id "messages/unreliable-unordered" :type :task :title "unreliable-unordered messages for time-sensitive data" :state :done :evidence ("test.cpp: test_connection_unreliable_unordered_messages" "test.cpp: test_single_message_type_unreliable") :tested-revision "3bdbaf2b5a669083c4dcf8d77a939e063567468b")
  (:id "messages/blocks" :type :task :title "data blocks larger than a packet on reliable-ordered messages" :state :done :evidence ("test.cpp: test_connection_reliable_ordered_blocks" "test.cpp: test_connection_reliable_ordered_blocks_max_size" "test.cpp: test_single_message_type_reliable_blocks") :tested-revision "3bdbaf2b5a669083c4dcf8d77a939e063567468b")
  (:id "messages/unreliable-blocks" :type :task :title "data blocks on unreliable-unordered messages" :state :done :evidence ("test.cpp: test_connection_unreliable_unordered_blocks") :tested-revision "3bdbaf2b5a669083c4dcf8d77a939e063567468b")
  (:id "messages/multiple-channels" :type :task :title "multiple channels on one connection" :state :done :evidence ("test.cpp: test_connection_reliable_ordered_messages_and_blocks_multiple_channels") :tested-revision "3bdbaf2b5a669083c4dcf8d77a939e063567468b")
  (:id "messages/serialize" :type :task :title "bitpacker and serialization system" :state :done :evidence ("test.cpp: test_bit_array" "test.cpp: test_client_server_message_failed_to_serialize_reliable_ordered" "test.cpp: test_client_server_message_failed_to_serialize_unreliable_unordered") :tested-revision "3bdbaf2b5a669083c4dcf8d77a939e063567468b")
  (:id "messages/queue-limits" :type :task :title "full send and receive queues fail cleanly" :state :done :evidence ("test.cpp: test_client_server_message_receive_queue_overflow" "test.cpp: test_client_server_message_exhaust_stream_allocator") :tested-revision "3bdbaf2b5a669083c4dcf8d77a939e063567468b")
  (:id "network" :type :roadmap :title "Networking"
   :text "Packets on the wire, the transports they ride, and the tools for testing under bad networks."
   :rows (("encrypted" "encrypted and signed packets over UDP")
          ("fragmentation" "packet fragmentation and reassembly")
          ("addresses" "IPv4 and IPv6 addresses")
          ("custom-io" "custom packet I/O through the Adapter")
          ("simulator" "network simulator: latency, jitter, loss, duplicates")
          ("network-info" "per-connection latency, jitter, loss and bandwidth estimates")
          ("standard" "wire format specified in STANDARD.md and checked against the code"))
   :columns (("done" "done"))
   :cells (("encrypted" "done" "network/encrypted")
           ("fragmentation" "done" "network/fragmentation")
           ("addresses" "done" "network/addresses")
           ("custom-io" "done" "network/custom-io")
           ("simulator" "done" "network/simulator")
           ("network-info" "done" "network/network-info")
           ("standard" "done" "network/standard")))
  (:id "network/encrypted" :type :task :title "encrypted and signed packets over UDP" :state :done :evidence ("test.cpp: test_crypto_aead_vectors" ".github/workflows/ci.yml: sodium-parity.yml parity job") :tested-revision "3bdbaf2b5a669083c4dcf8d77a939e063567468b")
  (:id "network/fragmentation" :type :task :title "packet fragmentation and reassembly" :state :unknown
   :note "Configured through reliable (fragmentPacketsAbove, packetFragmentSize), but no yojimbo test sends a packet above fragmentPacketsAbove and checks it reassembles; such a test makes this done.")
  (:id "network/addresses" :type :task :title "IPv4 and IPv6 addresses" :state :done :evidence ("test.cpp: test_address" "test.cpp: test_address_classification" "test.cpp: test_address_malformed_port") :tested-revision "3bdbaf2b5a669083c4dcf8d77a939e063567468b")
  (:id "network/custom-io" :type :task :title "custom packet I/O through the Adapter" :state :done :evidence ("custom_packet_io_test.cpp: main (ctest yojimbo_custom_packet_io_test)" "commit 3812f63 (Expose custom packet I/O through Adapter (#328))") :tested-revision "3bdbaf2b5a669083c4dcf8d77a939e063567468b")
  (:id "network/simulator" :type :task :title "network simulator: latency, jitter, loss, duplicates" :state :done :evidence ("test.cpp: test_network_simulator_drains_all_slots" "test.cpp: test_client_server_messages" "test.cpp: test_client_server_messages_network_sim_leak") :tested-revision "3bdbaf2b5a669083c4dcf8d77a939e063567468b")
  (:id "network/network-info" :type :task :title "per-connection latency, jitter, loss and bandwidth estimates" :state :unknown
   :note "GetNetworkInfo is only called by test_interface_methods_link, which checks it links, not its values; a test that asserts the estimates under the simulator makes this done.")
  (:id "network/standard" :type :task :title "wire format specified in STANDARD.md and checked against the code" :state :done :evidence (".github/workflows/ci.yml: conformance job, Verify STANDARD.md" "commit ace723e (conformance: check yojimbo's STANDARD.md against the implementation)") :tested-revision "3bdbaf2b5a669083c4dcf8d77a939e063567468b")
  (:id "library" :type :roadmap :title "Library"
   :text "Building, installing and embedding yojimbo, on every platform it supports."
   :rows (("platforms" "Linux, macOS and Windows")
          ("startup-failure" "startup refuses cleanly when an allocation fails")
          ("abi" "one ABI across debug and release builds")
          ("allocators" "custom allocators, TLSF by default")
          ("install" "installs consumable with find_package, bundled or against system dependencies")
          ("subproject" "embeddable with add_subdirectory")
          ("netcode-floor" "a netcode below the security floor is refused at configure time")
          ("strict-fp" "the wire builds with -ffp-contract=off on every target")
          ("matcher" "matcher: a sample web backend that issues connect tokens")
          ("docs" "API documentation built on every change, warnings are errors")
          ("docs-complete" "every public declaration documented (WARN_IF_UNDOCUMENTED on)"))
   :columns (("done" "done"))
   :cells (("platforms" "done" "library/platforms")
           ("startup-failure" "done" "library/startup-failure")
           ("abi" "done" "library/abi")
           ("allocators" "done" "library/allocators")
           ("install" "done" "library/install")
           ("subproject" "done" "library/subproject")
           ("netcode-floor" "done" "library/netcode-floor")
           ("strict-fp" "done" "library/strict-fp")
           ("matcher" "done" "library/matcher")
           ("docs" "done" "library/docs")
           ("docs-complete" "done" "library/docs-complete")))
  (:id "library/platforms" :type :task :title "Linux, macOS and Windows" :state :done :evidence (".github/workflows/ci.yml: build-test job (linux x86-64, macOS arm64) and windows job (MSVC x64 Debug and Release), green on 3bdbaf2") :tested-revision "3bdbaf2b5a669083c4dcf8d77a939e063567468b")
  (:id "library/startup-failure" :type :task :title "startup refuses cleanly when an allocation fails" :state :done :evidence ("test.cpp: test_server_start_alloc_failure" "test.cpp: test_client_connect_alloc_failure" "test.cpp: test_connection_reliable_message_alloc_failure") :tested-revision "3bdbaf2b5a669083c4dcf8d77a939e063567468b")
  (:id "library/abi" :type :task :title "one ABI across debug and release builds" :state :done :evidence ("test.cpp: test_public_layout_is_configuration_independent" "test.cpp: test_rtti_across_the_abi_boundary" "commit f7cb323 (Library contracts: transactional startup, one ABI, consumable installs (YJ-01 to YJ-12) (#340))") :tested-revision "3bdbaf2b5a669083c4dcf8d77a939e063567468b")
  (:id "library/allocators" :type :task :title "custom allocators, TLSF by default" :state :done :evidence ("test.cpp: test_allocator_tlsf") :tested-revision "3bdbaf2b5a669083c4dcf8d77a939e063567468b")
  (:id "library/install" :type :task :title "installs consumable with find_package, bundled or against system dependencies" :state :done :evidence (".github/workflows/ci.yml: bundled-install-consumer job" ".github/workflows/ci.yml: system-deps job, Install and build a clean-prefix consumer") :tested-revision "3bdbaf2b5a669083c4dcf8d77a939e063567468b")
  (:id "library/subproject" :type :task :title "embeddable with add_subdirectory" :state :done :evidence (".github/workflows/ci.yml: bundled-install-consumer job, Embed yojimbo with add_subdirectory") :tested-revision "3bdbaf2b5a669083c4dcf8d77a939e063567468b")
  (:id "library/netcode-floor" :type :task :title "a netcode below the security floor is refused at configure time" :state :done :evidence (".github/workflows/ci.yml: system-deps job, The netcode floor is enforced for consumers") :tested-revision "3bdbaf2b5a669083c4dcf8d77a939e063567468b")
  (:id "library/strict-fp" :type :task :title "the wire builds with -ffp-contract=off on every target" :state :done :evidence ("test.cpp: test_fp_contract_off" "CMakeLists.txt: ctest yojimbo_check_compile_commands_fp") :tested-revision "3bdbaf2b5a669083c4dcf8d77a939e063567468b")
  (:id "library/matcher" :type :task :title "matcher: a sample web backend that issues connect tokens" :state :done :evidence (".github/workflows/ci.yml: matcher job, The image serves a connect token") :tested-revision "3bdbaf2b5a669083c4dcf8d77a939e063567468b")
  (:id "library/docs" :type :task :title "API documentation built on every change, warnings are errors" :state :done :evidence (".github/workflows/ci.yml: docs job") :tested-revision "3bdbaf2b5a669083c4dcf8d77a939e063567468b")
  (:id "library/docs-complete" :type :task :title "every public declaration documented (WARN_IF_UNDOCUMENTED on)" :state :todo
   :note "Document every public declaration, then turn WARN_IF_UNDOCUMENTED on in doxygen.config."
   :github-issue "341")
  (:id "testing" :type :roadmap :title "Testing and security"
   :text "How yojimbo is checked on every change, and the hardening work still open."
   :rows (("sanitizers" "ASan, UBSan and LSan on every change")
          ("fuzz" "fuzzed packet, token and connection parsers")
          ("soak" "soak test")
          ("caa-gate" "pull requests gated on the Contributor Assignment Agreement")
          ("caa-pinned" "the CAA action is pinned to a commit and holds a narrowed credential")
          ("caa-past" "past contributors have signed the Contributor Assignment Agreement"))
   :columns (("done" "done"))
   :cells (("sanitizers" "done" "testing/sanitizers")
           ("fuzz" "done" "testing/fuzz")
           ("soak" "done" "testing/soak")
           ("caa-gate" "done" "testing/caa-gate")
           ("caa-pinned" "done" "testing/caa-pinned")
           ("caa-past" "done" "testing/caa-past")))
  (:id "testing/sanitizers" :type :task :title "ASan, UBSan and LSan on every change" :state :done :evidence (".github/workflows/ci.yml: sanitizers job") :tested-revision "3bdbaf2b5a669083c4dcf8d77a939e063567468b")
  (:id "testing/fuzz" :type :task :title "fuzzed packet, token and connection parsers" :state :done :evidence (".github/workflows/ci.yml: fuzz-build job" ".github/workflows/fuzz-nightly.yml: nightly fuzz job") :tested-revision "3bdbaf2b5a669083c4dcf8d77a939e063567468b")
  (:id "testing/soak" :type :task :title "soak test" :state :done :evidence (".github/workflows/ci.yml: soak job") :tested-revision "3bdbaf2b5a669083c4dcf8d77a939e063567468b")
  (:id "testing/caa-gate" :type :task :title "pull requests gated on the Contributor Assignment Agreement" :state :done :evidence (".github/workflows/cla.yml" "commit 66ea057 (CAA: stop failing on issue threads, and stop dropping signatures silently (#312))") :tested-revision "3bdbaf2b5a669083c4dcf8d77a939e063567468b")
  (:id "testing/caa-pinned" :type :task :title "the CAA action is pinned to a commit and holds a narrowed credential" :state :todo
   :note "Audit SEC-02 (2026-09-04): cla.yml runs the archived contributor-assistant/github-action@v2.6.1 by mutable tag with a cross-repository PAT; pin it to a full SHA, then replace it and split the credentials. The audit's netcode and reliable findings belong to those repositories."
   :github-issue "339")
  (:id "testing/caa-past" :type :task :title "past contributors have signed the Contributor Assignment Agreement" :state :todo
   :note "Requests to past contributors to sign the CAA so ownership of yojimbo is consolidated."
   :github-issue "306"
   :github-issue-2 "331")))
