(defproject louis "0.1.0-SNAPSHOT"
  :dependencies [[org.clojure/clojure "1.9.0"]
                 [org.graalvm.sdk/graal-sdk "19.0.2"]]
  :repositories {"local" "file:mvn"}
  :target-path "lein"
  :profiles {:step1 {:java-source-paths ["java/pre"]
                     :source-paths ["clj"]
                     :aot :all}
             :step2 {:java-source-paths ["java/post"]}
             :step3 {:auto-clean false}})
