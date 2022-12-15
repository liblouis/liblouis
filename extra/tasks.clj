(ns tasks
  (:require
   [babashka.process :refer [process shell]]
   [clojure.string :as str]))

;; majorly inspired by the Announce script from git, see
;; https://github.com/git/git/blob/ecaae0c78185918cb24292407354f58c043b6ca8/Announce

(defn latest-release []
  (->> "git describe --abbrev=0"
      (shell {:out :string})
      :out
      str/trim))

(defn authors
  ([] (authors (latest-release)))
  ([release]
   (->> release
        (format "git shortlog -s --no-merges --group=author --group=trailer:co-authored-by %s..")
        (shell {:out :string})
        :out
        str/split-lines
        (map (fn [line] (-> line (str/split #"\t") second)))
        (sort-by str/lower-case)
        (str/join ", "))))

(defn changes-since
  ([] (changes-since (latest-release)))
  ([release]
   (-> (process (format "git log --no-merges %s.." release))
       (process {:out :string} "git shortlog")
       deref
       :out)))

